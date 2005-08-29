/*	common/crc.h
 * 
 *	vi: set autoindent tabstop=8 shiftwidth=4 :
 */

#ifndef	_CRC_H
#define	_CRC_H

/*	calculate a 32-bit crc	*/
extern void do_crc(uint8_t * data, uint32_t len, uint32_t * result);

#endif
