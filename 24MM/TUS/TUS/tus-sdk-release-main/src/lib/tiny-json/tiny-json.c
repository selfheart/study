/*
  <github.com/rafagafe/tiny-json>

  Licensed under the MIT License <opensource.org/licenses/MIT>.
  SPDX-License-Identifier: MIT
  Copyright (c) 2016-2018 Rafa Garcia <rafagarcia77@gmail.com>.

  Permission is hereby  granted, free of charge, to any  person obtaining a copy
  of this software and associated  documentation files (the "Software"), to deal
  in the Software  without restriction, including without  limitation the rights
  to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
  copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
  FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
  AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
  LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

##################################################
    With modifications by Airbiquity 2021
##################################################
*/


#include <string.h>
#include <ctype.h>
#include "tiny-json.h"

/* parasoft-begin-suppress CERT_C-MSC41-a-1 "This string does not contain sensitive information."  */
/* parasoft-begin-suppress CERT_C-DCL06-a-3 "Keeping hard coded values in favor of code readability" */
/* parasoft-begin-suppress CERT_C-DCL02-a-3 "Keeping camelCase formatting in favor of code readability" */

/* parasoft-begin-suppress MISRAC2012-RULE_17_8-a-4 MISRAC2012-RULE_15_5-a-4 CERT_C-API00-a-3 Maintaining tiny-json program flow */

/* parasoft-begin-suppress MISRAC2012-RULE_15_1-a-4  MISRAC2012-RULE_15_4-a-4
 *  ABQ validated that all uses of 'goto' meet the requirements in order to allow the use of 'goto'
 */

/* parasoft-begin-suppress MISRAC2012-RULE_18_4-a-4 MISRAC2012-DIR_4_1-i-2 MISRAC2012-RULE_18_2-a-2 CERT_C-ARR36-a-2 CERT_C-ARR39-b-2 CERT_C-EXP08-a-2 "Exempting pointer arithmetic as it is a foundation to the Tiny Json library code" */



/** Structure to handle a heap of JSON properties. */
typedef struct jsonStaticPool_s {
    json_t *mem;       /**< Pointer to array of json properties.      */
    uint32_t qty;      /**< Length of the array of json properties.   */
    uint32_t nextFree; /**< The index of the next free json property. */
    jsonPool_t pool;
} jsonStaticPool_t;

/* Search a property by its name in a JSON object. */
json_t const *json_getProperty(json_t const *const obj, byte_t const *const property) {
    json_t const *sibling;
    for (sibling = obj->u.c.child; sibling != NULL; sibling = sibling->sibling) {
        if (sibling->name && !strcmp(sibling->name, property)) { return sibling; }
    }
    return NULL;
}

/* Internal prototypes: */
static byte_t *goBlank(byte_t *const str);

static jsonErr_t goBlank_ex(byte_t **const out, byte_t *const str, const byte_t *const end_point);

static byte_t *goNum(byte_t *str);

static json_t *poolInit(jsonPool_t *const pool);

static json_t *poolAlloc(jsonPool_t *const pool);

static jsonErr_t objValue_ex(byte_t **out, byte_t *ptr, const byte_t *const end_point, json_t *obj,
                             jsonPool_t *const pool);

static byte_t *setToNull(byte_t *ch);

static bool isEndOfPrimitive(const byte_t ch);

/* Parse a string to get a json with length check. */
static jsonErr_t json_createWithPool_ex(json_t const **const out, byte_t *const str, const byte_t *const end_point,
                                 jsonPool_t *const pool) {
    byte_t *ptr = NULL;
    byte_t *ret_ptr = NULL;
    jsonErr_t ret = JSON_NO_ERROR;
    ret = goBlank_ex(&ptr, str, end_point);
    if ((NULL == ptr) || ((*ptr != '{') && (*ptr != '['))) {
        goto error;
    }
    json_t *const obj = pool->init(pool);
    obj->name = NULL;
    obj->sibling = NULL;
    obj->u.c.child = NULL;
    ret = objValue_ex(&ret_ptr, ptr, end_point, obj, pool);
    if (NULL == ret_ptr) {
        goto error;
    } else {
        *out = obj;
    }
error:
    return ret;
}

/* Parse a string to get a json with length check. */
jsonErr_t json_create_ex(json_t const **const out, byte_t *const str, const uint64_t length,
                         json_t mem[], const uint32_t qty) {
    jsonStaticPool_t spool;

    spool.mem = mem;
    spool.qty = qty;
    spool.pool.init = poolInit;
    spool.pool.alloc = poolAlloc;

    return json_createWithPool_ex(out, str, &str[length], &spool.pool);
}


/** Parse a string and replace the scape characters by their meaning characters.
  * This parser stops when finds the character '\"'. Then replaces '\"' by '\0'.
  * @param str Pointer to first character.
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *parseString(byte_t *const str) {
    uint8_t *head = cast_f1(str);
    uint8_t *tail = head;
    while ((uint8_t) '\0' != *head) {
        if (*head == (uint8_t) '\"') {
            *tail = (uint8_t) '\0';
            head = &head[1];
            return (byte_t *) cast_f2(head);
        }
        if ((*head == (uint8_t) '\\') && (((head[1] == (uint8_t) '\"')) || (head[1] == (uint8_t) '\\'))) {
            *tail = *head;
            head = &head[1];
            tail = &tail[1];
            *tail = *head;
        } else {
            *tail = *head;
        }

        head = &head[1];
        tail = &tail[1];
    }
    return NULL;
}

/** Parse a string to get the name of a property.
  * @param ptr Pointer to first character.
  * @param property The property to assign the name.
  * @retval Pointer to first of property value. If success.
  * @retval Null pointer if any error occur. */
static byte_t *propertyName(byte_t *ptr, json_t *const property) {
    ptr = &ptr[1];
    property->name = ptr;
    ptr = parseString(ptr);
    if (NULL == ptr) { return NULL; }
    ptr = goBlank(ptr);
    if (NULL == ptr) { return NULL; }
    if (*ptr != ':') {ptr = &ptr[1]; return NULL; }
    ptr = &ptr[1];

    return goBlank(ptr);
}

/** Parse a string to get the value of a property when its type is JSON_TEXT.
  * @param ptr Pointer to first character ('\"').
  * @param property The property to assign the name.
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *textValue(byte_t *ptr, json_t *const property) {
    property->u.value = &property->u.value[1];
    ptr = &ptr[1];
    ptr = parseString(ptr);
    if (NULL == ptr) { return NULL; }
    property->type = JSON_TEXT;
    return ptr;
}

/** Compare two strings until get the null character in the second one.
  * @param ptr sub string
  * @param str main string
  * @retval Pointer to next character.
  * @retval Null pointer if any error occur. */
static byte_t *checkStr(byte_t *ptr, byte_t const *str) {
    while ('\0' != *str) {
        if (*ptr != *str) {ptr = &ptr[1]; str = &str[1]; return NULL; }
        ptr = &ptr[1]; str = &str[1];
    }
    return ptr;
}

/** Parser a string to get a primitive value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @param value String with the primitive literal.
  * @param type The code of the type. ( JSON_BOOLEAN or JSON_NULL )
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *primitiveValue(byte_t *ptr, json_t *const property, byte_t const *const value, const jsonType_t type) {
    ptr = checkStr(ptr, value);
    if ((NULL == ptr) || (!isEndOfPrimitive(*ptr))) { return NULL; }
    ptr = setToNull(ptr);
    property->type = type;
    return ptr;
}

/** Parser a string to get a true value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *trueValue(byte_t *const ptr, json_t *const property) {
    return primitiveValue(ptr, property, "true", JSON_BOOLEAN);
}

/** Parser a string to get a false value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *falseValue(byte_t *const ptr, json_t *const property) {
    return primitiveValue(ptr, property, "false", JSON_BOOLEAN);
}

/** Parser a string to get a null value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *nullValue(byte_t *const ptr, json_t *const property) {
    return primitiveValue(ptr, property, "null", JSON_NULL);
}

/** Analyze the exponential part of a real number.
  * @param ptr Pointer to first character.
  * @retval Pointer to first non numerical after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *expValue(byte_t *ptr) {
    if ((*ptr == '-') || (*ptr == '+')) { ptr = &ptr[1]; }
    if ((((int32_t)*ptr < ((int32_t)'0')) || ((int32_t)*ptr > ((int32_t)'9')))) { return NULL; }
    ptr = &ptr[1];
    ptr = goNum(ptr);
    return ptr;
}

/** Analyze the decimal part of a real number.
  * @param ptr Pointer to first character.
  * @retval Pointer to first non numerical after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *fraqValue(byte_t *ptr) {
    if ((((int32_t)*ptr < ((int32_t)'0')) || ((int32_t)*ptr > ((int32_t)'9')))) { return NULL; }
    ptr = &ptr[1];
    ptr = goNum(ptr);
    if (NULL == ptr) { return NULL; }
    return ptr;
}

/** Parser a string to get a numerical value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type: JSON_REAL or JSON_INTEGER.
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static byte_t *numValue(byte_t *ptr, json_t *const property) {
    byte_t *org_ptr;
    org_ptr = ptr;
    if (*ptr == '-') { ptr = &ptr[1]; }
    if ((((int32_t)*ptr < ((int32_t)'0')) || ((int32_t)*ptr > ((int32_t)'9')))) { return NULL; }
    if (*ptr != '0') {
        ptr = goNum(ptr);
        if (NULL == ptr) { return NULL; }
    } else {
        ptr = &ptr[1];
        if ((((int32_t)*ptr >= ((int32_t)'0')) && ((int32_t)*ptr <= ((int32_t)'9')))) { return NULL; }
    }
    property->type = JSON_INTEGER;
    if (*ptr == '.') {
        ptr = &ptr[1];
        ptr = fraqValue(ptr);
        if (NULL == ptr) { return NULL; }
        property->type = JSON_REAL;
    }
    if ((*ptr == 'e') || (*ptr == 'E')) {
        ptr = &ptr[1];
        ptr = expValue(ptr);
        if (NULL == ptr) { return NULL; }
        property->type = JSON_REAL;
    }
    if (!isEndOfPrimitive(*ptr)) { return NULL; }
    if (JSON_INTEGER == property->type) {
        const byte_t *const value = property->u.value;
        bool const negative = ((uint8_t) *value == (uint8_t) '-');
        static byte_t const min[] = "-9223372036854775808";
        static byte_t const max[] = "9223372036854775807";
        int64_t const maxdigits = ((true == negative) ? (int64_t) sizeof(min) : (int64_t)(((int64_t) sizeof(max)) - (int64_t)1U));
        int64_t const len = (int64_t const)(ptr - org_ptr);
        if (len > maxdigits) { return NULL; }
        if (len == maxdigits) {
            byte_t const tmp = *ptr;
            *ptr = '\0';
            byte_t const *const threshold = (true == negative) ? min : max;
            if (0 > strcmp(threshold, value)) { return NULL; }
            *ptr = tmp;
        }
    }
    ptr = setToNull(ptr);
    return ptr;
}

/** Add a property to a JSON object or array.
  * @param obj The handler of the JSON object or array.
  * @param property The handler of the property to be added. */
static void add(json_t *const obj, json_t *const property) {
    property->sibling = NULL;
    if (NULL == obj->u.c.child) {
        obj->u.c.child = property;
        obj->u.c.last_child = property;
    } else {
        obj->u.c.last_child->sibling = property;
        obj->u.c.last_child = property;
    }
}

/** Parser a string to get a json object value with checking end point.
  * @param out Pointer to first character after the value. If success.
  *             Null pointer if any error occur.
  * @param ptr Pointer to first character.
  * @param end_point Pointer to last character.
  * @param obj The handler of the JSON root object or array.
  * @param pool The handler of a json pool for creating json instances.
  * @return #JSON_ERR_INVALID_PARAMETER Any was wrong in the parse process.
  * @return #JSON_NO_ERROR The parser process was successfully.
  * @return #JSON_ERR_NO_RESOURCE JSON string has a value which exceeds expected length.
  */
static jsonErr_t objValue_ex(byte_t **out, byte_t *ptr, const byte_t *const end_point, json_t *obj, jsonPool_t *const pool) {
    jsonErr_t ret = JSON_NO_ERROR;
    byte_t *tmp_out = NULL;
    obj->type = (*ptr == '{') ? JSON_OBJ : JSON_ARRAY;
    obj->u.c.child = NULL;
    obj->sibling = NULL;
    ptr = &ptr[1];
    for (;;) {
        ret = goBlank_ex(&tmp_out, ptr, end_point);
        if (NULL == tmp_out) {
            goto error;
        } else {
            ptr = tmp_out;
        }
        if (*ptr == ',') {
            ptr = &ptr[1];
            continue;
        }
        byte_t const endchar = (obj->type == JSON_OBJ) ? '}' : ']';
        if (*ptr == endchar) {
            *ptr = '\0';
            json_t *const parentObj = obj->sibling;
            if (NULL == parentObj) {
                ptr = &ptr[1];
                *out = ptr;
                ret = JSON_NO_ERROR;
                goto error;
            }
            obj->sibling = NULL;
            obj = parentObj;
            ptr = &ptr[1];
            continue;
        }
        json_t *const property = pool->alloc(pool);
        if (NULL == property) {
            ret = JSON_ERR_NO_RESOURCE;
            goto error;
        }

        if (obj->type != JSON_ARRAY) {
            if (*ptr != '\"') {
                ret = JSON_ERR_INVALID_PARAMETER;
                goto error;
            }
            ptr = propertyName(ptr, property);
            if (NULL == ptr) {
                ret = JSON_ERR_INVALID_PARAMETER;
                goto error;
            }
        } else {
            property->name = NULL;
        }
        add(obj, property);
        property->u.value = ptr;
        switch (*ptr) {
            case '{':
                property->type = JSON_OBJ;
                property->u.c.child = NULL;
                property->sibling = obj;
                obj = property;
                ptr = &ptr[1];
                break;
            case '[':
                property->type = JSON_ARRAY;
                property->u.c.child = NULL;
                property->sibling = obj;
                obj = property;
                ptr = &ptr[1];
                break;
            case '\"':
                ptr = textValue(ptr, property);
                break;
            case 't':
                ptr = trueValue(ptr, property);
                break;
            case 'f':
                ptr = falseValue(ptr, property);
                break;
            case 'n':
                ptr = nullValue(ptr, property);
                break;
            default:
                ptr = numValue(ptr, property);
                break;
        }
        if (NULL == ptr) {
            ret = JSON_ERR_INVALID_PARAMETER;
            goto error;
        }
    }
    error:
    if (ret != JSON_NO_ERROR) {
        out = NULL;
    }
    return ret;
}


/** Initialize a json pool.
  * @param pool The handler of the pool.
  * @return a instance of a json. */
static json_t *poolInit(jsonPool_t *const pool) {
    jsonStaticPool_t *spool = json_containerOf(pool, jsonStaticPool_t, pool);
    spool->nextFree = 1;
    return spool->mem;
}

/** Create an instance of a json from a pool.
  * @param pool The handler of the pool.
  * @retval The handler of the new instance if success.
  * @retval Null pointer if the pool was empty. */
static json_t *poolAlloc(jsonPool_t *const pool) {
    jsonStaticPool_t *spool = json_containerOf(pool, jsonStaticPool_t, pool);
    if (spool->nextFree >= spool->qty) { return NULL; }
    json_t * const tmp = &spool->mem[spool->nextFree];
    spool->nextFree++;
    return tmp;
}

/** Checks whether an character belongs to set.
  * @param ch Character value to be checked.
  * @param set Set of characters. It is just a null-terminated string.
  * @return true or false there is membership or not. */
static bool isOneOfThem(const byte_t ch, byte_t const *set) {
    while (*set != '\0') {
        if (ch == *set) {set = &set[1]; return true; }
        set = &set[1];
    }
    return false;
}

/** Increases a pointer while it points to a character that belongs to a set.
  * @param str The initial pointer value.
  * @param set Set of characters. It is just a null-terminated string.
  * @return The final pointer value or null pointer if the null character was found. */
static byte_t *goWhile(byte_t *str, byte_t const *const set) {
    if (NULL != str) {
        for (; *str != '\0'; str = &str[1]) {
            if (!isOneOfThem(*str, set)) { return str; }
        }
    }
    return NULL;
}

/** Increases a pointer while it points to a character that belongs to a set
  * with checking end point.
  * @param out The final pointer value or null pointer if the null character was found.
  * @param str The initial pointer value.
  * @param end_point The last pointer value.
  * @param set Set of characters. It is just a null-terminated string.
  * @return #JSON_ERR_INVALID_PARAMETER Any was wrong in the parse process.
  * @return #JSON_NO_ERROR The parser process was successfully.
  * @return #JSON_ERR_NO_RESOURCE JSON string has a value which exceeds expected length.
  */
static jsonErr_t goWhile_ex(byte_t **out, byte_t *str, const byte_t *const end_point, byte_t const *const set) {
    jsonErr_t ret = JSON_NO_ERROR;
    if (NULL != str) {
        while ((*str != '\0') && (str != end_point)) {
            if (!isOneOfThem(*str, set)) {
                *out = str;
                ret = JSON_ERR_INVALID_PARAMETER;
                goto error;
            }
            str = &str[1];
        }
    }
    error:
    if (ret != JSON_NO_ERROR) {
        out = NULL;
    }
    return ret;
}

/** Set of characters that defines a blank. */
static byte_t const *const blank = " \n\r\t\f";

/** Increases a pointer while it points to a white space character.
  * @param str The initial pointer value.
  * @return The final pointer value or null pointer if the null character was found. */
static byte_t *goBlank(byte_t *const str) {
    return goWhile(str, blank);
}

/** Increases a pointer while it points to a white space character
  * with checking end point.
  * @param out The final pointer value or null pointer if the null character was found.
  * @param str The initial pointer value.
  * @param end_point The last pointer value.
  * @return #JSON_ERR_INVALID_PARAMETER Any was wrong in the parse process.
  * @return #JSON_NO_ERROR The parser process was successfully.
  * @return #JSON_ERR_NO_RESOURCE JSON string has a value which exceeds expected length.
  */
static jsonErr_t goBlank_ex(byte_t **const out, byte_t *const str, const byte_t *const end_point) {
    return goWhile_ex(out, str, end_point, blank);
}

/** Increases a pointer while it points to a decimal digit character.
  * @param str The initial pointer value.
  * @return The final pointer value or null pointer if the null character was found. */
static byte_t *goNum(byte_t *str) {
    for (; (*str != '\0'); str = &str[1]) {
        if ((((int32_t)*str < ((int32_t)'0')) || ((int32_t)*str > ((int32_t)'9')))) { return str; }
    }
    return NULL;
}

/** Set of characters that defines the end of an array or a JSON object. */
static byte_t const *const endofblock = "}]";

/** Set a byte_t to '\0' and increase its pointer if the byte_t is different to '}' or ']'.
  * @param ch Pointer to character.
  * @return  Final value pointer. */
static byte_t *setToNull(byte_t *ch) {
    if (!isOneOfThem(*ch, endofblock)) {
        *ch = '\0';
        ch = &ch[1];
    }
    return ch;
}

/** Indicate if a character is the end of a primitive value. */
static bool isEndOfPrimitive(const byte_t ch) {
    return ((ch == ',') || (isOneOfThem(ch, blank)) || (isOneOfThem(ch, endofblock)));
}

/* parasoft-end-suppress MISRAC2012-RULE_15_1-a-4  MISRAC2012-RULE_15_4-a-4 */
/* parasoft-end-suppress MISRAC2012-RULE_17_8-a-4  MISRAC2012-RULE_15_5-a-4 CERT_C-API00-a-3 */
/* parasoft-end-suppress CERT_C-MSC41-a-1 */
/* parasoft-end-suppress CERT_C-DCL06-a-3 */
/* parasoft-end-suppress CERT_C-DCL02-a-3 */
/* parasoft-end-suppress MISRAC2012-RULE_18_4-a-4 MISRAC2012-DIR_4_1-i-2 MISRAC2012-RULE_18_2-a-2 CERT_C-ARR36-a-2 CERT_C-ARR39-b-2 CERT_C-EXP08-a-2 */
