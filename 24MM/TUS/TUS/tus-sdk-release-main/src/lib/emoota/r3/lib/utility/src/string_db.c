/* ****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#include "string_db.h"
/* parasoft-begin-suppress MISRAC2012-DIR_4_9-a-4 MISRAC2012-RULE_2_5-a-4 MISRAC2012-RULE_20_10-a-4 MISRAC2012-RULE_20_5-a-4 CERT_C-PRE00-a-3
 *  Use of X-macro is not function like. X-macro is used in header file macro ICD_LIST_STRING
 *  preprocessor # is used to ensure no typos
 */

/* parasoft-begin-suppress CERT_C-MSC41-a-1 "This string does not contain sensitive information."  */
/* parasoft-begin-suppress CERT_C-DCL06-a-3 Keeping hard coded values in favor of code readability */
/* parasoft-begin-suppress CERT_C-DCL02-a-3 "Keeping camelCase formatting in favor of code readability" */

#define NUM_OF_DB_ENTRIES  (233U)

#define X(value)    {.string_value=#value}
struct for_string_db {
    cstr_t const string_value;
} static const string_db[NUM_OF_DB_ENTRIES] = {      // parasoft-suppress MISRAC2012-RULE_8_9-a-4
        ICD_LIST_STRING
};
#undef X

 /* parasoft-end-suppress MISRAC2012-DIR_4_9-a-4 MISRAC2012-RULE_2_5-a-4 MISRAC2012-RULE_20_10-a-4 MISRAC2012-RULE_20_5-a-4 CERT_C-PRE00-a-3 */

 cstr_t getDBstr(const string_db_access_key_t db_key) {
     cstr_t rval = NULL;
     if ((uint16_t)db_key <= NUM_OF_DB_ENTRIES) {
         rval = string_db[db_key].string_value;
     }
     return rval;
}

/* parasoft-end-suppress CERT_C-MSC41-a-1 */
/* parasoft-end-suppress CERT_C-DCL06-a-3 */
/* parasoft-end-suppress CERT_C-DCL02-a-3 */


