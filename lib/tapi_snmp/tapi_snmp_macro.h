/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * SNMP protocol implementaion internal declarations.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Renata Sayakhova <Renata.Sayakhova@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_TAPI_SNMP_MACRO_H__
#define __TE_TAPI_SNMP_MACRO_H__ 

#include "tapi_snmp.h"
#include "tapi_test.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Macro around tapi_snmp_csap_create().
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF Session ID.
 * @param snmp_agent    Address of SNMP agent.
 * @param community     SNMP community.
 * @param snmp_version  SNMP version.
 * @param csap_id       identifier of an SNMP CSAP. (OUT)
 * 
 */
#define SNMP_CSAP_CREATE(ta_, sid_, snmp_agent_, community_, snmp_version_, csap_id_) \
    do                                                                                \
    {                                                                                 \
        int rc_;                                                                      \
	                                                                              \
        rc_ = tapi_snmp_csap_create(ta_, sid_, snmp_agent_, community_,               \
                                    snmp_version_, &csap_id_);                        \
        if (rc_ != 0)                                                                 \
	{                                                                             \
            TEST_FAIL("snmp csap creation failed %X\n", rc_);                         \
        }                                                                             \
    } while (0)

/**
 * Macro around tapi_snmp_make_oid().
 * 
 * @param label_    SNMP label - OID string representation
 * @param oid_      Location for parsed OID (OUT)
 * 
 */ 
#define SNMP_MAKE_OID(label_, oid_)                                                   \
    do                                                                                \
    {                    	                                                      \
        int rc_;                                                                      \
	                                                                              \
        rc_ = tapi_snmp_make_oid(label_, &oid_);                                      \
	if (rc_ != 0)                                                                 \
	{                                                                             \
            TEST_FAIL("snmp make integer failed for OID, result\n", label_, rc_);     \
        }                                                                             \
    } while (0)	    

/**
 * Macro around tapi_snmp_make_oid().
 * 
 * @param label_    SNMP label - OID string representation
 * @param oid_      Location for parsed OID (OUT)
 * 
 */ 
#define TAPI_SNMP_MAKE_OID(label_, oid_) \
    CHECK_RC(tapi_snmp_make_oid(label_, oid_))

/**
 * Append one SUB ID to OID (macro around tapi_snmp_oid_append()).
 *
 * @param oid_     OID to be appended
 * @param len_     The number of sub ids to add
 * @param subid_   SUB ID value
 */
#define TAPI_SNMP_CREATE_OID(oid_, len_, sub_ids_...) \
    do {                                              \
        tapi_snmp_zero_oid(oid_);                     \
        tapi_snmp_append_oid(oid_, len_, sub_ids_);   \
    } while (0)

/**
 * Append one SUB ID to OID (macro around tapi_snmp_oid_append()).
 *
 * @param oid_     OID to be appended
 * @param subid_   SUB ID value
 */
#define TAPI_SNMP_APPEND_OID_SUBID(oid_, subid_) \
    tapi_snmp_append_oid(oid_, 1, subid_)


/**
 * Macro around tapi_snmp_oid_zero().
 *
 * @param oid_     OID to be cleared
 */
#define TAPI_SNMP_OID_ZERO(oid_) \
    tapi_snmp_oid_zero(oid_)

/**
 * Macro around tapi_snmp_make_table_field_instance().
 * 
 * @param label_    SNMP label - OID string representation
 * @param oid_      Location for parsed OID (OUT)
 * @param ...       Indeces of table field instance
 * 
 */ 
#define SNMP_MAKE_INSTANCE(label_, oid_, indeces_...) \
    do {                       	                                          \
        int rc_;                                                          \
                                                                          \
        rc_ = tapi_snmp_make_instance(label_, &oid_, indeces_);           \
        if (rc_ != 0)                                                     \
        {                                                                 \
            TEST_FAIL("Cannot make instance of %s OID: %X", label_, rc_); \
        }                                                                 \
    } while (0)

#define TAPI_SNMP_MAKE_VB(vb_, oid_, type_, value_...) \
        CHECK_RC(tapi_snmp_make_vb(vb_, oid_, type_, value_))

/**
 * Macro around tapi_snmp_get_syntax().
 * 
 * @param label_    SNMP label - OID string representation
 * @param syntax_   Location for syntax 
 * 
 */ 
#define TAPI_SNMP_GET_SYNTAX(label_, syntax_) \
    do                                                                      \
    {                    	                                            \
        int             rc_;                                                \
        tapi_snmp_oid_t oid_;                                               \
	                                                                    \
        TAPI_SNMP_MAKE_OID(label_, &oid_);                                  \
	                                                                    \
        rc_ = tapi_snmp_get_syntax(&oid_, syntax_);                         \
	if (rc_ != 0)                                                       \
	{                                                                   \
            TEST_FAIL("snmp get syntax failed  OID, result\n", label_, rc_);\
        }                                                                   \
    } while (0)	    

/**
 * Macro around tapi_snmp_get_table().
 * 
 * @param ta_            Test Agent name
 * @param sid_           RCF Session id
 * @param csap_id_       SNMP CSAP handle
 * @param label_         OID of SNMP table Entry object, or one leaf in this 
 *                       entry
 * @param num_           Number of raws in table = height of matrix below (OUT)
 * @param result_        Allocated matrix with results, if only 
 *                       one column should be get, matrix width is 1, otherwise 
 *                       matrix width is greatest subid of Table entry (OUT)
 *
 */
#define TAPI_SNMP_GET_TABLE(ta_, sid_, csap_id_, label_, num_, result_) \
    do {                                                                \
        int             rc_;                                            \
        tapi_snmp_oid_t oid_;                                           \
	                                                                \
        TAPI_SNMP_MAKE_OID(label_, &oid_);                              \
 	rc_ = tapi_snmp_get_table(ta_, sid_, csap_id_, &oid_,           \
                                  num_, (void **)result_);              \
	if (rc_ != 0)                                                   \
	{                                                               \
            TEST_FAIL("snmp get table for %s failed, result %X\n",      \
	              label_, rc_);                                     \
        }                                                               \
    } while (0)

/**
 * Extracts subtable so that all the entries in the subtable have
 * specified prefix in their index.
 *
 * @param tbl_           Pointer to the SNMP table data structure
 * @param tbl_size_      Number of rows in the table
 * @param index_prefix_  Index prefix that should present in all rows of 
 *                       the resulting subtable (tapi_snmp_oid_t *)
 * @param sub_tbl_       Placeholder for the pointer to the subtable (OUT)
 * @param sub_tbl_size_  Number of rows in the subtable (OUT)
 *
 * @note If there is no entry with specified index prefix,
 *       sub_tbl_ is set to NULL, and sub_tbl_size_ is set to zero.
 *
 * @example
 *
 * 
 */
#define TAPI_SNMP_GET_SUBTABLE(tbl_, tbl_size_, index_prefix_, \
                               sub_tbl_, sub_tbl_size_)                      \
    do {                                                                     \
          int i_;                                                            \
                                                                             \
          *(sub_tbl_size_) = 0;                                              \
          *(sub_tbl_) = NULL;                                                \
          for (i_ = 0; i_ < (tbl_size_); i_++)                               \
          {                                                                  \
              if (tbl_[i_].index_suffix->length < (index_prefix_)->length)   \
              {                                                              \
                  TEST_FAIL("Row %d in the table has index whose length is " \
                            "less than the length of passed index prefix "); \
              }                                                              \
              if (memcmp((tbl_)[i_].index_suffix->id, (index_prefix_)->id,   \
                         (index_prefix_)->length) == 0)                      \
              {                                                              \
                  if (*(sub_tbl_size_) == 0)                                 \
                  {                                                          \
                      /* We've found the first row with the index prefix */  \
                      *(sub_tbl_) = &(tbl_)[i_];                             \
                  }                                                          \
                  (*(sub_tbl_size_))++;                                      \
              }                                                              \
          }                                                                  \
      } while (0)
   
/**
 * Macro around tapi_snmp_get_table_rows().
 *
 * @param ta_            Test Agent name
 * @param sid_           RCF Session id
 * @param csap_id_       SNMP CSAP handle
 * @param label_         OID of SNMP table Entry MIB node
 * @param num_           number of suffixes
 * @param suffixes_      Array with index suffixes of desired table rows
 * @param result_        Allocated matrix with results,
 *                       matrix width is greatest subid of Table entry (OUT)
 * 
 */
#define SNMP_GET_TABLE_ROWS(ta_, sid_, csap_id_, label_, num_, \
                            suffixes_, result_)                    \
    do {                                                           \
        int             rc_;                                       \
        tapi_snmp_oid_t oid_;                                      \
	                                                           \
        TAPI_SNMP_MAKE_OID(label_, oid_);                          \
 	rc_ = tapi_snmp_get_table_rows(ta_, sid_, csap_id_, &oid_, \
                                       num_, suffixes_, &result_); \
	if (rc_ != 0)                                              \
	{                                                          \
            TEST_FAIL("snmp get table rows failed for %s failed, " \
                      "result %X\n", label_, rc_);                 \
        }                                                          \
    } while (0)        	    


/**
 * Macro around tapi_snmp_get_table_dimension().
 *
 * @param label_      OID of SNMP table Entry object, or one leaf in this
 *                    entry
 * @param dimension_  Pointer to the location of the table dimension (OUT)
 */ 
#define TAPI_SNMP_GET_TABLE_DIMENSION(label_, dimension_) \
    do {                                                                 \
        int             rc_;                                             \
        tapi_snmp_oid_t oid_;                                            \
	                                                                 \
        TAPI_SNMP_MAKE_OID(label_, &oid_);                               \
 	rc_ = tapi_snmp_get_table_dimension(&oid_, dimension_);          \
	if (rc_ != 0)                                                    \
	{                                                                \
            TEST_FAIL("snmp get table dimension failed for %s failed, "  \
                      "result %X\n", label_, rc_);                       \
        }                                                                \
    } while (0)

/**
 * Macro around tapi_snmp_get_table_columns().
 *
 * @param label_     OID of SNMP table Entry object, or one leaf in this
 *                   entry
 * @param columns_   Columns of the table.
 *
 */ 
#define TAPI_SNMP_GET_TABLE_COLUMNS(label_, columns_) \
    do                                                                     \
    {                                                                      \
        int             rc_;                                               \
        tapi_snmp_oid_t oid_;                                              \
	                                                                   \
        TAPI_SNMP_MAKE_OID(label_, &oid_);                                 \
 	rc_ = tapi_snmp_get_table_columns(&oid_, &columns_);               \
	if (rc_ != 0)                                                      \
	{                                                                  \
            TEST_FAIL("snmp get table columns failed for %s failed, "      \
                      "result %X\n", label_, rc_);                         \
        }                                                                  \
    } while (0)
  
   
/**
 * Macro around tapi_snmp_load_mib_with_path().
 *
 * @param dir_path_  Path to directory with MIB files
 * @param mib_file_  File name of the MIB file
 *
 */
#define SNMP_LOAD_MIB_WITH_PATH(dir_path_, mib_file_) \
    do {                                                                   \
       	int rc_;                                                           \
                                                                           \
        rc_ = tapi_snmp_load_mib_with_path(dir_path_,                      \
			                   mib_file_);                     \
	if (rc_ != 0)                                                      \
        {                                                                  \
            TEST_FAIL("Loading mib with path failed, result %X\n", rc_);   \
        }                                                                  \
    } while (0)


/**
 * Macro around tapi_snmp_load_mib().
 *
 * @param mib_file_  File name of the MIB file.
 * 
 */
#define SNMP_LOAD_MIB(mib_file_) \
    do {                                                       \
       	int rc_;                                               \
                                                               \
        rc_ = tapi_snmp_load_mib(mib_file_);                   \
	if (rc_ != 0)                                          \
        {                                                      \
            TEST_FAIL("Loading mib failed, result %X\n", rc_); \
        }                                                      \
    } while (0)

#define SNMP_MAKE_TBL_INDEX(label_, index_... ) \
    do                                                         \
    {                                                          \
	tapi_snmp_oid_t label_;                                \
	int             rc_;                                   \
	TAPI_SNMP_MAKE_OID(#label_, label_);                   \
        rc_ = tapi_snmp_make_table_index(&label_, &index_);    \
	if (rc_)                                               \
            TEST_FAIL("Make table index failed\n");            \
    } while (0)	
	
   

/**
 * Macro around tapi_snmp_set_integer()
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param name           name of an SNMP object the value is to be set.
 * @param value          integer value.
 * @param err_stat       SNMP error status
 * @param sub_id         index of table field instance or 0 for scalar field.
 * 
 */
#define TAPI_SNMP_SET_INTEGER(ta, sid, csap_id, name, value,                   \
                              err_stat, sub_id...)                             \
    do {                                                                       \
        tapi_snmp_oid_t           leaf_oid;                                    \
                                                                               \
        CHECK_RC(tapi_snmp_make_instance(name, &leaf_oid, sub_id));            \
        CHECK_RC(tapi_snmp_set_integer((ta), (sid), (csap_id), &leaf_oid,      \
                                       (value), (err_stat)));                  \
	VERB("SNMP set integer, set %s to %d, error status %d",                \
	     name, value, *err_stat);                                          \
    } while (0)

/**
 * Macro around tapi_snmp_set_octetstring()
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param name           name of an SNMP object the value is to be set.
 * @param value          octet string value.
 * @param length         octet string length.
 * @param err_stat       SNMP error status
 * @param sub_id         index of table field instance or 0 for scalar field.
 * 
 */
#define TAPI_SNMP_SET_OCTETSTRING(ta, sid, csap_id, name,                      \
                                  value, length, err_stat, sub_id...)          \
    do {                                                                       \
        tapi_snmp_oid_t           leaf_oid;                                    \
                                                                               \
        CHECK_RC(tapi_snmp_make_instance(name, &leaf_oid, sub_id));            \
        CHECK_RC(tapi_snmp_set_octetstring((ta), (sid), (csap_id), &leaf_oid,  \
                                       (value), (length), (err_stat)));        \
	VERB("SNMP set octetstring, set %s to %s, error status %d", name,      \
             print_octet_string(value, length), *err_stat);                    \
    } while (0)

/**
 * Macro for snmp set string type variable. 
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param name           name of an SNMP object the value is to be set.
 * @param value          display string value.
 * @param err_stat       SNMP error status
 * @param sub_id         index of table field instance or 0 for scalar field.
 * 
 */
#define TAPI_SNMP_SET_STRING(ta, sid, csap_id, name, value,                    \
                             err_stat, sub_id...)                              \
    TAPI_SNMP_SET_OCTETSTRING(ta, sid, csap_id, name, value,                   \
                              strlen(value), err_stat, sub_id)

/**
 * Macro around tapi_snmp_set_row()
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param err_stat       SNMP error status
 * @param err_index      index of varbind where an error ocured
 * @param index          index of table field instance or 0 for scalar field.
 * 
 */
#define TAPI_SNMP_SET_ROW(ta, sid, csap_id, err_stat, err_index, index,        \
                          values...)                                           \
    do {                                                                       \
        CHECK_RC(tapi_snmp_set_row(ta, sid, csap_id, err_stat, err_index,      \
                                   index, values));                            \
        VERB("SNMP set row, error status %d, error index %d",                  \
             *err_stat, *err_index);                                           \
    } while (0)

/**
 * Macro around tapi_snmp_set()
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param err_stat       SNMP error status
 * @param err_index      index of varbind where an error ocured
 * 
 */
#define TAPI_SNMP_SET(ta, sid, csap_id, err_stat, err_index, values...) \
    do {                                                                \
        CHECK_RC(tapi_snmp_set(ta, sid, csap_id,                        \
                               err_stat, err_index, values));           \
    } while (0)


/**
 * Macro around tapi_snmp_get()
 *
 * @param ta             Test Agent name
 * @param sid            RCF Session id
 * @param csap_id        identifier of an SNMP CSAP
 * @param name           name of an SNMP object the value is to be set
 * @param next           GetRequest or GetNextRequest
 * @param vb             Location for returned varbind
 * @param subid          index of table field instance (0 for scalar field)
 *
 */
#define TAPI_SNMP_GET(ta, sid, csap_id, name, next, vb, err_stat, sub_id...) \
    do                                                              \
    {                                                               \
        tapi_snmp_oid_t          oid;                               \
	                                                            \
        CHECK_RC(tapi_snmp_make_instance(name, &oid, sub_id));      \
	CHECK_RC(tapi_snmp_get(ta, sid, csap_id, &oid,              \
                 next, vb, err_stat));                              \
	VERB("SNMP get for %s, oid = %s,  error status %d",         \
             name, print_oid(&oid), *err_stat);                     \
    } while (0)	    

    
/**
 * Macro around tapi_snmp_get_integer()
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param name           name of an SNMP object the value is to be set.
 * @param value          pointer to returned integer value.
 * @param err_stat	 error status
 * @param subid          index of table field instance (0 for scalar field).
 * 
 */
#define TAPI_SNMP_GET_INTEGER(ta, sid, csap_id, name, value, err_stat, sub_id...) \
    do                                                                         \
    {                                                                          \
        tapi_snmp_oid_t     oid;                                               \
	                                                                       \
        CHECK_RC(tapi_snmp_make_instance(name, &oid, sub_id));                 \
        CHECK_RC(tapi_snmp_get_integer(ta, sid, csap_id, &oid,                 \
                 (value), err_stat));                                          \
	                                                                       \
        VERB("SNMP get: for %s (oid=%s) returns %s = %d, error status %d\n",   \
             name, print_oid(&oid), #value, (value), *err_stat);               \
    } while (0)


/**
 * Macro around tapi_snmp_get_octetstring()
 *
 * @param ta	         Test Agent name
 * @param sid            RCF Session id
 * @param csap_id        identifier of an SNMP CSAP
 * @param name           name of an SNMP object the value is to be set
 * @param value          Location for returned value
 * @param size	         size of returned value
 * @param err_stat       error status
 * @param subid          index of table field instance (0 for scalar field)
 *
 */ 
#define TAPI_SNMP_GET_OCTETSTRING(ta, sid, csap_id, name, value,               \
                                  size, err_stat, sub_id...)                   \
    do                                                                         \
    {                                                                          \
        tapi_snmp_oid_t       oid;                                             \
	                                                                           \
        CHECK_RC(tapi_snmp_make_instance(name, &oid, sub_id));                 \
        CHECK_RC(tapi_snmp_get_oct_string(ta, sid, csap_id, &oid,              \
                 (value), size, err_stat));                                    \
        VERB("SNMP get octetstring: for %s (oid = %s) "                        \
             "returns %s = %s, error status %d",                               \
             name, print_oid(&oid), #value,                                    \
             print_octet_string(value, *size), *err_stat);                     \
    } while (0) 	    

		
/**
 * Macro around tapi_snmp_walk.
 *
 * @param	ta		Test Agent name
 * @param	sid		RCF Session id
 * @param 	csap_id		identifier of an SNMP CSAP
 * @param	name		name of SNMP object
 * @param	userdata	userdata for walk cb
 * @param	callback	walk callback
 *
 */ 
#define TAPI_SNMP_WALK(ta, sid, csap_id, name, userdata, callback) \
    do                                                                        \
    {                                                                         \
        tapi_snmp_oid_t           oid;                                        \
                                                                              \
        CHECK_RC(tapi_snmp_make_oid(name, &oid));                             \
        CHECK_RC(tapi_snmp_walk(ta, sid, csap_id, &oid, userdata, callback)); \
    } while (0)


/**
 * Macro to check SNMP type integer variable.
 * 
 * @param ta             Test Agent name
 * @param sid            RCF Session id.
 * @param csap_id        identifier of an SNMP CSAP.
 * @param name           name of an SNMP object the value is to be set.
 * @param value          integer value to compare.
 * @param subid          index of table field instance (0 for scalar field).
 * 
 */
#define TAPI_SNMP_CHECK_INTEGER(ta, sid, csap_id, name, value, \
                                err_stat, sub_id...)                         \
    do                                                                       \
    {                                                                        \
        int                       tmp_value;                                 \
        tapi_snmp_oid_t           oid;                                       \
	                                                                     \
        CHECK_RC(tapi_snmp_make_instance(name, &oid, sub_id));               \
        CHECK_RC(tapi_snmp_get_integer(ta, sid, csap_id, &oid,               \
                 &tmp_value, err_stat));                                     \
        VERB("SNMP get: for %s (oid=%s) returns %s = %d, error status %d\n", \
             name, print_oid(&oid), #value, (value), *err_stat);             \
        if (value != tmp_value)                                              \
            TEST_FAIL("The value of %s instance is %d, but it is expected "  \
                      "to be %d", print_oid(&oid), tmp_value, value);        \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SNMP_MACRO_H__ */
