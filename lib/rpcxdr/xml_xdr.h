#ifndef __BASE_TYPE_CVT_H__
#define __BASE_TYPE_CVT_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>    
#include <features.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <stdio.h>
#include <rpc/xdr.h>    
#include <expat.h>    
    
#define MAXBUFSIZE      256
#define INDENT          2 

/**
 * From xml data representation point of view
 * 'enum' type is the 'int' one
 */ 
#define xmlxdr_enum xmlxdr_int    

/**
 * Tags distinguished for rpc call and
 * rpc result
 */ 
typedef enum rpc_xml_op {    
    rpc_xml_call,
    rpc_xml_result
} rpc_xml_op;

/**
 * Additional data is to be passed to
 * conversion procedures
 */ 
typedef struct xml_app_data {
    rpc_xml_op  op;      /**< rpc_xml_call or rpc_xml_result */
    bool_t      rc;      /**< return code */
    const char *type;    /**< data type */
    const char *name;    /**< data name */
    int         depth;   /**< embedding level */
    XML_Parser  parser;  /**< XML parser */
} xml_app_data;

/**
 * Conversion procedures for base data types
 */
extern bool_t xmlxdr_uint8_t(XDR *xdrs, uint8_t *ip);
extern bool_t xmlxdr_int(XDR *xdrs, int *ip);
extern bool_t xmlxdr_uint32_t(XDR *xdrs, uint32_t *ip);
extern bool_t xmlxdr_int32_t(XDR *xdrs, int32_t *ip);
extern bool_t xmlxdr_uint16_t(XDR *xdrs, uint16_t *ip);
extern bool_t xmlxdr_int16_t(XDR *xdrs, int16_t *ip);
extern bool_t xmlxdr_uint64_t(XDR *xdrs, uint64_t *ip);
extern bool_t xmlxdr_char(XDR *xdrs, char *ip);
extern bool_t xmlxdr_array(XDR *xdrs, caddr_t *addrp, u_int *sizep,
                           u_int maxsize, u_int elsize, xdrproc_t elproc);
extern bool_t xmlxdr_vector(XDR *xdrs, char *basep, u_int nelem,
                            u_int elsize, xdrproc_t elproc);
extern bool_t xmlxdr_string(XDR *xdrs, caddr_t *addrp, u_int maxsize);

/**
 * Parser handlers for structure data types 
 */
extern void start_compound_data(void *data, const XML_Char *elem, 
                                const XML_Char **atts);
extern void end_compound_data(void *data, const XML_Char *elem);

/**
 * Initializes XDR structure to perform
 * further convertions.
 *
 * @param xdrs      XDR structure
 * @param buf       buffer to contain xml document representing
 *                  transmitting data
 * @param buflen    buffer size
 * @param op        rpc_xml_call or rpc_xml_result
 * @param rc        TRUE of FALSE, used only when op is rpc_xml_result
 * @param name      call/result name
 * @param x_op      XDR_ENCODE, XDR_DECODE of XDR_FREE
 *
 * @return TRUE of FALSE (in case of memory allocation failure)
 */ 
extern bool_t xdrxml_create(XDR *xdrs, caddr_t buf, u_int buflen, 
                            rpc_xml_op op, bool_t rc,
                            const char *name, enum xdr_op x_op);
/**
 * Free memory allocated during XDR structure initializing in
 * xdrxml_create()
 *
 * @param xdrs  XDR structure
 */ 
extern bool_t xdrxml_return_code(XDR *xdrs);

/**
 * Return rc containing in xml document
 * attached to XDR structure
 *
 * @param xdrs  XDR structure
 */ 
extern bool_t xdrxml_free(XDR *xdrs);

#ifdef __cplusplus
}
#endif
#endif

