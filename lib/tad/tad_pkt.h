/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Packets Representation
 *
 * Traffic Application Domain Packets Representation definitions
 * of data types and provided functions.
 *
 * TAD packet is a list of segments. Each segments contains pointer and
 * length of its data plus function to be used to free these data.
 * Pointer and length are opaque parameters for TAD generic support.
 * Each protocol (its callbacks) deals with these data. However, packets
 * representation library provides some helper functions to allocate
 * memory for segments data. In the last case memory array of specified
 * length is allocated.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_PKT_H__
#define __TE_TAD_PKT_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"

#include "tad_common.h"


#ifdef __cplusplus
extern "C" {
#endif


/** TAD packet segment */
typedef struct tad_pkt_seg tad_pkt_seg;

/** TAD packet representation */
typedef struct tad_pkt tad_pkt;

/** TAD list of packets */
typedef struct tad_pkts tad_pkts;

/**
 * Packet segment data free function prototype.
 *
 * @param ptr       Pointer
 * @param len       Associated length
 */
typedef void (*tad_pkt_seg_free)(void *ptr, size_t len);

/**
 * Trivial wrapper for free() to match tad_pkt_seg_free prototype.
 */
extern void tad_pkt_seg_data_free(void *ptr, size_t len);


/**
 * Prototype of the function to free packet representation control
 * blocks (segment, packet).
 *
 * @param ptr       Pointer to control block to be freed
 */
typedef void (*tad_pkt_ctrl_free)(void *ptr);


/**
 * Forward traversal of TAD packets list.
 *
 * @param _head     Pointer to head of the list (tad_pkts_head *)
 * @param _pkt      Loop varaible to assign pointer to a packet
 *                  (tad_pkt *)
 */
#define TAD_PKT_FOR_EACH_PKT_FWD(_head, _pkt) \
    for ((_pkt) = (_head)->cqh_first;       \
         (_pkt) != (void *)(_head);         \
         (_pkt) = (_pkt)->links.cqe_next)   \

/**
 * Reverse traversal of TAD packets list.
 *
 * @param _head     Pointer to head of the list (tad_pkts_head *)
 * @param _pkt      Loop varaible to assign pointer to a packet
 *                  (tad_pkt *)
 */
#define TAD_PKT_FOR_EACH_PKT_REV(_head, _pkt) \
    for ((_pkt) = (_head)->cqh_last;        \
         (_pkt) != (void *)(_head);         \
         (_pkt) = (_pkt)->links.cqe_prev)   \


/**
 * Forward traversal of TAD packet segments.
 *
 * @param _head     Pointer to head of the list (tad_pkt_segs *)
 * @param _seg      Loop varaible to assign pointer to a segment
 *                  (tad_seg_seg *)
 */
#define TAD_PKT_FOR_EACH_SEG_FWD(_head, _seg) \
    for ((_seg) = (_head)->cqh_first;       \
         (_seg) != (void *)(_head);         \
         (_seg) = (_seg)->links.cqe_next)   \


/**
 * TAD packet segment control block.
 *
 * Circular queue is used to have possibility to insert element in head
 * and tail and to traverse segments in both directions.
 */
struct tad_pkt_seg {
    CIRCLEQ_ENTRY(tad_pkt_seg)  links;  /**< List links */

    void               *data_ptr;      /**< Data pointer */
    size_t              data_len;      /**< Data length */
    tad_pkt_seg_free    data_free;     /**< Data free function */

    tad_pkt_ctrl_free   my_free;       /**< Function to free this
                                            control block */

    te_bool             layer_tag_set; /**< Custom layer tag is set below */
    te_tad_protocols_t  layer_tag;     /**< TE protocol ID of the layer from
                                            which the segment originates */
};

/** Head of packet segments list */
typedef CIRCLEQ_HEAD(tad_pkt_segs, tad_pkt_seg) tad_pkt_segs;

/**
 * TAD packet control block.
 *
 * Circular queue is used to have possibility to insert element in head
 * and tail and to traverse packets in both directions.
 */
struct tad_pkt {
    CIRCLEQ_ENTRY(tad_pkt)  links;  /**< List links */

    tad_pkt_segs        segs;       /**< Packet segments */
    unsigned int        n_segs;     /**< Number of segments in
                                         the packet */
    size_t              segs_len;   /**< Total length of all segments
                                         in the packet */

    tad_pkt_ctrl_free   my_free;    /**< Function to free this
                                         control block */

    void               *opaque;         /**< Attached opaque data */
    tad_pkt_ctrl_free   opaque_free;    /**< Function to free opaque
                                             data */
};

/** Head of packets list */
typedef CIRCLEQ_HEAD(tad_pkts_head, tad_pkt) tad_pkts_head;

/**
 * List of packets.
 */
struct tad_pkts {
    tad_pkts_head   pkts;   /**< Packets */
    unsigned int    n_pkts; /**< Number of packets in the list */
};



/**
 * Initialize segment to be added in a packet.
 *
 * @param seg       Segment which does not belong to any packet
 * @param ptr       Pointer to a data
 * @param len       Length of data
 * @param free      Function to be called to free these data
 */
extern void tad_pkt_init_seg_data(tad_pkt_seg *seg,
                                  void *ptr, size_t len,
                                  tad_pkt_seg_free free);

/**
 * Free packet segment data.
 *
 * @param seg       Segment to free its data
 */
extern void tad_pkt_free_seg_data(tad_pkt_seg *seg);

/**
 * Free packet segment.
 *
 * @param seg       Segment to be freed
 */
extern void tad_pkt_free_seg(tad_pkt_seg *seg);

/**
 * Allocate packet segment.
 *
 * @param data_ptr      Pointer to segment data or NULL, if array
 *                      of specified length should be allocated
 * @param data_len      Length of data in data_ptr or to be allocated
 * @param data_free     Function to be used to free @a data_ptr, if it
 *                      is not a NULL
 *
 * @return Packet segment pointer or NULL.
 */
extern tad_pkt_seg *tad_pkt_alloc_seg(void *data_ptr, size_t data_len,
                                      tad_pkt_seg_free data_free);


/**
 * Get number of segments in a packet.
 *
 * @param pkt       Pointer to a packet
 *
 * @return Number of segments in the packet.
 */
extern unsigned int tad_pkt_seg_num(const tad_pkt *pkt);

/**
 * Get total length of a packet.
 *
 * It makes sence if packet segments refer to simple memory array of
 * specified length.
 *
 * @param pkt       Pointer to a packet
 *
 * @return Sum of all segments length.
 */
extern size_t tad_pkt_len(const tad_pkt *pkt);

/**
 * Get opaque data pointer of the packet.
 *
 * @param pkt       Pointer to a packet
 *
 * @return Opaque data pointer.
 */
extern void *tad_pkt_opaque(const tad_pkt *pkt);

/**
 * Set opaque data pointer of the packet.
 *
 * @param pkt           Pointer to a packet
 * @param opaque        Opaque pointer
 * @param opaque_free   Function to free opaque data
 */
extern void tad_pkt_set_opaque(tad_pkt *pkt, void *opaque,
                               tad_pkt_ctrl_free opaque_free);

/**
 * Initialize packet as empty (no segments).
 *
 * @param pkt       Pointer to a packet
 */
extern void tad_pkt_init_segs(tad_pkt *pkt);

/**
 * Free packet segments.
 *
 * @param pkt       Pointer to a packet
 */
extern void tad_pkt_free_segs(tad_pkt *pkt);

/**
 * Initialize packet.
 *
 * @param pkt           Pointer to a packet
 * @param my_free       Function to free packet itself
 * @param opaque        Opaque data
 * @param opaque_free   Function to free opaque data
 */
extern void tad_pkt_init(tad_pkt *pkt, tad_pkt_ctrl_free my_free,
                         void *opaque, tad_pkt_ctrl_free opaque_free);

/**
 * Free all data resources, but keep control structures.
 *
 * @param pkt       Packet to be cleaned up
 */
extern void tad_pkt_cleanup(tad_pkt *pkt);

/**
 * Free packet.
 *
 * @param pkt       Pointer to a packet
 */
extern void tad_pkt_free(tad_pkt *pkt);

/**
 * Allocate a packet with specified number of segments and
 * length of the first segment data.
 *
 * @param n_segs        Number of segments to allocate for each packet
 * @param first_seg_len Length of the first segment of each packet
 *
 * @return Allocated packet or NULL.
 */
extern tad_pkt *tad_pkt_alloc(unsigned int n_segs, size_t first_seg_len);


/**
 * Put data in the packet segment.
 *
 * @param pkt       Packet
 * @param seg       Segment of the packet
 * @param ptr       Pointer to a data to put
 * @param len       Length of data
 * @param free      Function to be called to free these data
 */
extern void tad_pkt_put_seg_data(tad_pkt *pkt, tad_pkt_seg *seg,
                                 void *ptr, size_t len,
                                 tad_pkt_seg_free free);

/**
 * Set new length of the packet segment data.
 *
 * @param pkt       Packet
 * @param seg       Segment
 * @param new_len   New value of the segment data length
 */
extern void tad_pkt_set_seg_data_len(tad_pkt *pkt, tad_pkt_seg *seg,
                                     size_t new_len);

/**
 * Add segment to the packet as trailer.
 *
 * @param pkt       Packet
 * @param seg       Segment to add
 */
extern void tad_pkt_append_seg(tad_pkt *pkt, tad_pkt_seg *seg);

/**
 * Add segment to the packet as header.
 *
 * @param pkt       Packet
 * @param seg       Segment to add
 */
extern void tad_pkt_prepend_seg(tad_pkt *pkt, tad_pkt_seg *seg);

/**
 * Add segment to the packet.
 *
 * @param pkt       Packet
 * @param seg       Segment to insert after it
 * @param new_seg   Segment to insert
 */
extern void tad_pkt_insert_after_seg(tad_pkt *pkt, tad_pkt_seg *seg,
                                     tad_pkt_seg *new_seg);

/**
 * Make flatten copy of a packet to specified memory array or to
 * allocated using malloc().
 *
 * @param pkt       Packet
 * @param data      Location of the pointer to memory array
 *                  (if *data is NULL, allocate it using malloc())
 * @param len       Location of the memory array length or NULL
 *                  (if *data is not NULL, *len is equal to size
 *                  of buffer provided by caller)
 *
 * If buffer provided by caller is not sufficient, the buffer is filled
 * in and TE_ESMALLBUF is returned.
 *
 * @return Status code.
 */
extern te_errno tad_pkt_flatten_copy(tad_pkt *pkt,
                                     uint8_t **data, size_t *len);

/**
 * Get the first segment of the packet.
 *
 * @param pkt       Packet
 *
 * @return Pointer to the first segment or NULL.
 */
static inline tad_pkt_seg *
tad_pkt_first_seg(const tad_pkt *pkt)
{
    assert(pkt != NULL);
    return (pkt->segs.cqh_first != (void *)&pkt->segs) ?
               pkt->segs.cqh_first : NULL;
}

/**
 * Get the last segment of the packet.
 *
 * @param pkt       Packet
 *
 * @return Pointer to the last segment or NULL.
 */
static inline tad_pkt_seg *
tad_pkt_last_seg(const tad_pkt *pkt)
{
    assert(pkt != NULL);
    return (pkt->segs.cqh_last != (void *)&pkt->segs) ?
               pkt->segs.cqh_last : NULL;
}

/**
 * Get the next segment of the packet.
 *
 * @param pkt       Packet
 *
 * @return Pointer to the next segment or NULL.
 */
static inline tad_pkt_seg *
tad_pkt_next_seg(const tad_pkt *pkt, const tad_pkt_seg *seg)
{
    assert(pkt != NULL);
    assert(seg != NULL);
    assert(seg != (void *)&pkt->segs);
    return (seg->links.cqe_next != (void *)&pkt->segs) ?
               seg->links.cqe_next : NULL;
}

/**
 * Get the previous segment of the packet.
 *
 * @param pkt       Packet
 *
 * @return Pointer to the previous segment or NULL.
 */
static inline tad_pkt_seg *
tad_pkt_prev_seg(const tad_pkt *pkt, const tad_pkt_seg *seg)
{
    assert(pkt != NULL);
    assert(seg != NULL);
    assert(seg != (void *)&pkt->segs);
    return (seg->links.cqe_prev != (void *)&pkt->segs) ?
               seg->links.cqe_prev : NULL;
}

/**
 * Prototype of the function to be called for each packet segment.
 *
 * @param pkt       Packet
 * @param seg       Segment
 * @param seg_num   Number of the segment in packet (starting from 0)
 * @param opaque    Opaque pointer passed to enumeration routine
 *
 * @return Status code.
 */
typedef te_errno (* tad_pkt_seg_enum_cb)(const tad_pkt  *pkt,
                                         tad_pkt_seg    *seg,
                                         unsigned int    seg_num,
                                         void           *opaque);

/**
 * Enumerate packet segments.
 *
 * @param pkt       Packet
 * @param func      Function to be called for each segment
 * @param opaque    Opaque pointer to be passed to callback function
 *
 * @return Status code.
 */
extern te_errno tad_pkt_enumerate_seg(const tad_pkt       *pkt,
                                      tad_pkt_seg_enum_cb  func,
                                      void                *opaque);

/**
 * Put packet segments data pointers with lengths to IO vector.
 *
 * @param[in]   pkt     Packet
 * @param[out]  iov     IO vector
 * @param[in]   count   Number of element in @a iov
 *
 * @return Status code.
 */
extern te_errno tad_pkt_segs_to_iov(const tad_pkt *pkt,
                                    struct iovec *iov, size_t count);


/**
 * Get number of packets in the list.
 *
 * @param pkts      Pointer to the list of packets
 *
 * @return Number of packet in the list.
 */
extern unsigned int tad_pkts_get_num(const tad_pkts *pkts);

/**
 * Initialize list of packets.
 *
 * @param pkts      Pointer to the list of packets
 */
extern void tad_pkts_init(tad_pkts *pkts);

/**
 * Free all data resources, but keep control structures.
 *
 * @param pkts      List of packets to be cleaned up
 */
extern void tad_cleanup_pkts(tad_pkts *pkts);

/**
 * Free packets.
 *
 * @param pkts      Pointer to the list of packets
 */
extern void tad_free_pkts(tad_pkts *pkts);

/**
 * Get the first packet from the list.
 *
 * @param pkts      List of packets
 *
 * @return Pointer to the first packet or NULL.
 */
static inline tad_pkt *
tad_pkts_first_pkt(const tad_pkts *pkts)
{
    assert(pkts != NULL);
    return (pkts->pkts.cqh_first != (void *)&pkts->pkts) ?
               pkts->pkts.cqh_first : NULL;
}

/**
 * Add packet to the list.
 *
 * @param pkts      List with packets
 * @param pkt       Packet to add
 */
extern void tad_pkts_add_one(tad_pkts *pkts, tad_pkt *pkt);

/**
 * Delete packet from the list.
 *
 * @param pkts      List with packets
 * @param pkt       Packet to delete
 */
extern void tad_pkts_del_one(tad_pkts *pkts, tad_pkt *pkt);

/**
 * Move packets from one list to other.
 *
 * @param dst       Destination list with packets
 * @param src       Source list with packets
 */
extern void tad_pkts_move(tad_pkts *dst, tad_pkts *src);

/**
 * Add a new segment to each packet in the list.
 *
 * @param pkts          List of packets
 * @param header        Add segment as header or trailer
 * @param data_ptr      Pointer to segment data or NULL, if array
 *                      of specified length should be allocated
 * @param data_len      Length of data in data_ptr or to be allocated
 * @param data_free     Function to be used to free @a data_ptr, if it
 *                      is not a NULL
 *
 * @return Status code.
 */
extern te_errno tad_pkts_add_new_seg(tad_pkts *pkts, te_bool header,
                                     void *data_ptr, size_t data_len,
                                     tad_pkt_seg_free data_free);

/**
 * Allocate a number of packets with specified number of segments and
 * length of the first segment data. Add allocated packets to the list.
 *
 * @param pkts          List to add allocated packets
 * @param n_pkts        Number of packets to allocate
 * @param n_segs        Number of segments to allocate for each packet
 * @param first_seg_len Length of the first segment of each packet
 *
 * @return Status code.
 */
extern te_errno tad_pkts_alloc(tad_pkts *pkts, unsigned int n_pkts,
                               unsigned int n_segs, size_t first_seg_len);


typedef te_errno (* tad_pkt_enum_cb)(tad_pkt *pkt, void *opaque);

/*
 * @return Status code.
 */
extern te_errno tad_pkt_enumerate(tad_pkts *pkts, tad_pkt_enum_cb func,
                                  void *opaque);

/**
 * Enumerate first segments of each packet in the list.
 *
 * @param pkts          List of packets
 * @param func          Function to be called for the first segment of
 *                      each packet
 * @param opaque        Pointer to opaque data to be passed in callback
 *                      function as the last argument
 *
 * @return Status code.
 */
extern te_errno tad_pkts_enumerate_first_segs(tad_pkts            *pkts,
                                              tad_pkt_seg_enum_cb  func,
                                              void                *opaque);


/**
 * Fragment a packet without data copying.
 *
 * @param pkt           Packet to fragment
 * @param frag_data_len Maximum length of the fragment (without any
 *                      additional segments length)
 * @param add_seg_len   Length of the additional segment or -1,
 *                      if no additional segments are necessary
 * @param header        Is additional segment header or trailer?
 * @param pkts          List to add fragments of the packet
 *
 * @return Status code.
 */
extern te_errno tad_pkt_fragment(tad_pkt *pkt, size_t frag_data_len,
                                 ssize_t add_seg_len, te_bool header,
                                 tad_pkts *pkts);


/** What to do, if more than available in packet is requested? */
typedef enum {
    TAD_PKT_GET_FRAG_ERROR, /**< Return an error */
    TAD_PKT_GET_FRAG_TRUNC, /**< Return less fragment */
    TAD_PKT_GET_FRAG_RAND,  /**< Add segment with randomly filled in data */
    TAD_PKT_GET_FRAG_ZERO,  /**< Add segment with data filled in by zeros */
} tad_pkt_get_frag_mode;

/**
 * Get (without copy of data) fragment of the packet.
 *
 * @param dst           Destination packet
 * @param src           Source packet
 * @param frag_off      Offset of the fragment in source packet
 * @param frag_len      Length of the fragment
 * @param mode          What to do, if more than available is requested?
 *
 * @return Status code.
 * @retval E2BIG        Too big fragment is requested with mode equal to
 *                      TAD_PKT_GET_FRAG_ERROR
 */
extern te_errno tad_pkt_get_frag(tad_pkt *dst, tad_pkt *src,
                                 size_t frag_off, size_t frag_len,
                                 tad_pkt_get_frag_mode mode);

/**
 * Read byte-aligned data from packet starting from offset in specified
 * segment. Offset have to fit in data available in the segment.
 * Packet have to have enoght data to read.
 *
 * @param pkt           Packet
 * @param seg           Packet segment
 * @param off           Offset in the segment
 * @param len           Total length of data to read
 * @param dst           Destination
 */
extern void tad_pkt_read(const tad_pkt *pkt, const tad_pkt_seg *seg,
                         size_t off, size_t len, uint8_t *dst);

/**
 * Read bits from packet. If length is not byte aligned, the bit string
 * is prepended by zero bits to be byte aligned.
 *
 * It is assumed that packet has enough data.
 *
 * @param pkt           Packet
 * @param bitoff        Offset in bits
 * @param bitlen        Number of bits to read starting from offset
 * @param dst           Destination
 */
extern void tad_pkt_read_bits(const tad_pkt *pkt, size_t bitoff,
                              size_t bitlen, uint8_t *dst);

/** Single-bit wrapper for @c tad_pkt_read_bits() */
extern te_bool tad_pkt_read_bit(const tad_pkt *pkt,
                                size_t         bitoff);

/**
 * Match packet content by mask.
 *
 * @param pkt           Packet
 * @param len           Length of the mask/value
 * @param mask          Mask bytes
 * @param value         Value bytes
 * @param exact_len     Is packet length should be equal to length of
 *                      the mask or may be greater?
 *
 * @return Status code.
 */
extern te_errno tad_pkt_match_mask(const tad_pkt *pkt, size_t len,
                                   const uint8_t *mask,
                                   const uint8_t *value,
                                   te_bool exact_len);

/**
 * Match packet content by payload.
 *
 * @param pkt           Packet
 * @param len           Length of the payload
 * @param mask          Payload bytes
 * @param exact_len     Is packet length should be equal to length of
 *                      the payload or may be greater?
 *
 * @return Status code.
 */
extern te_errno tad_pkt_match_bytes(const tad_pkt *pkt, size_t len,
                                    const uint8_t *payload,
                                    te_bool exact_len);

/**
 * Alloc additional segments for the packet
 *
 * @param pkt       the packet to add new segments
 * @param new_len   necessary length of the packet after adding segments
 *
 * @return Status code.
 */
extern te_errno tad_pkt_realloc_segs(tad_pkt *pkt, size_t new_len);

/**
 * A helper to set the given layer tag in the outermost TAD
 * packet segments which don't yet belong to any custom tag.
 *
 * This function complies with @c tad_pkt_enum_cb prototype
 * and must be invoked with @c tad_pkt_enumerate() facility.
 *
 * The opaque pointer must refer to the new tag value or be
 * equal to @c NULL. In the latter case the target segments
 * will be flagged as containing custom tag values although
 * the tag values themselves won't change from default ones.
 */
extern te_errno tad_pkt_mark_layer_segments_cb(tad_pkt *pkt,
                                               void    *opaque);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAD_PKT_H__ */
