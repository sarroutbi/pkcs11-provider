/* Copyright (c) 2022 Simo Sorce <simo@redhat.com> - see COPYING */

#include "provider.h"

int p11prov_fetch_attributes(CK_FUNCTION_LIST *f,
                             CK_SESSION_HANDLE session,
                             CK_OBJECT_HANDLE object,
                             struct fetch_attrs *attrs,
                             unsigned long attrnums)
{
    CK_ATTRIBUTE q[attrnums];
    CK_ATTRIBUTE r[attrnums];
    int ret;

    for (int i = 0; i < attrnums; i++) {
        if (attrs[i].allocate) {
            CKATTR_ASSIGN_ALL(q[i], attrs[i].type, NULL, 0);
        } else {
            CKATTR_ASSIGN_ALL(q[i], attrs[i].type,
                              *attrs[i].value,
                              *attrs[i].value_len);
        }
    }

    /* try one shot, then fallback to individual calls if that fails */
    ret = f->C_GetAttributeValue(session, object, q, attrnums);
    if (ret == CKR_OK) {
        unsigned long retrnums = 0;
        for (int i = 0; i < attrnums; i++) {
            if (q[i].ulValueLen == CK_UNAVAILABLE_INFORMATION) {
                if (attrs[i].required) {
                    return -ENOENT;
                }
                FA_RETURN_LEN(attrs[i], 0);
                continue;
            }
            if (attrs[i].allocate) {
                /* allways allocate and zero one more, so that
                 * zero terminated strings work automatically */
                char *a = OPENSSL_zalloc(q[i].ulValueLen + 1);
                if (a == NULL) return -ENOMEM;
                FA_RETURN_VAL(attrs[i], a, q[i].ulValueLen);

                CKATTR_ASSIGN_ALL(r[retrnums], attrs[i].type,
                                  *attrs[i].value,
                                  *attrs[i].value_len);
                retrnums++;
            } else {
                FA_RETURN_LEN(attrs[i], q[i].ulValueLen);
            }
        }
        if (retrnums > 0) {
            ret = f->C_GetAttributeValue(session, object, r, retrnums);
        }
    } else if (ret == CKR_ATTRIBUTE_SENSITIVE ||
               ret == CKR_ATTRIBUTE_TYPE_INVALID) {
        p11prov_debug("Quering attributes one by one\n");
        /* go one by one as this PKCS11 does not have some attributes
         * and does not handle it gracefully */
        for (int i = 0; i < attrnums; i++) {
            if (attrs[i].allocate) {
                CKATTR_ASSIGN_ALL(q[0], attrs[i].type, NULL, 0);
                ret = f->C_GetAttributeValue(session, object, q, 1);
                if (ret != CKR_OK) {
                    if (attrs[i].required) return ret;
                } else {
                    char *a = OPENSSL_zalloc(q[0].ulValueLen + 1);
                    if (a == NULL) return -ENOMEM;
                    FA_RETURN_VAL(attrs[i], a, q[0].ulValueLen);
                }
            }
            CKATTR_ASSIGN_ALL(r[0], attrs[i].type,
                              *attrs[i].value,
                              *attrs[i].value_len);
            ret = f->C_GetAttributeValue(session, object, r, 1);
            if (ret != CKR_OK) {
                if (r[i].ulValueLen == CK_UNAVAILABLE_INFORMATION) {
                    FA_RETURN_LEN(attrs[i], 0);
                }
                if (attrs[i].required) return ret;
            }
            p11prov_debug("Attribute| type:%lu value:%p, len:%lu\n",
                          attrs[i].type, *attrs[i].value, *attrs[i].value_len);
        }
        ret = CKR_OK;
    }
    return ret;
}

