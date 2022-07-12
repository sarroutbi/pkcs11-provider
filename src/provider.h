/* Copyright (c) 2022 Simo Sorce <simo@redhat.com> - see COPYING */

#ifndef _PROVIDER_H
#define _PROVIDER_H

#include "config.h"

#include <stdbool.h>

#include "pkcs11.h"
#include <openssl/core_dispatch.h>
#include <openssl/core_object.h>
#include <openssl/types.h>
#include <openssl/crypto.h>
#include <openssl/params.h>
#include <openssl/err.h>
#include <openssl/proverr.h>
#include <openssl/core_names.h>

#define UNUSED  __attribute__((unused))
#define RET_OSSL_OK 1
#define RET_OSSL_ERR 0
#define RET_OSSL_BAD -1

#define P11PROV_PKCS11_MODULE_PATH "pkcs11-module-path"
#define P11PROV_PKCS11_MODULE_INIT_ARGS "pkcs11-module-init-args"

#define P11PROV_DEFAULT_PROPERTIES "provider=pkcs11"
#define P11PROV_NAMES_RSA "PKCS11-RSA"
#define P11PROV_DESCS_RSA "PKCS11 RSA Implementation"
#define P11PROV_NAMES_ECDSA "PKCS11-ECDSA"
#define P11PROV_DESCS_ECDSA "PKCS11 ECDSA Implementation"
#define P11PROV_NAMES_ECDH "PKCS11-ECDH"
#define P11PROV_DESCS_ECDH "PKCS11 ECDH Implementation"
#define P11PROV_DESCS_URI "PKCS11 URI Store"

typedef struct st_provider_ctx PROVIDER_CTX;

struct p11prov_slot {
    CK_SLOT_ID id;
    CK_SLOT_INFO slot;
    CK_TOKEN_INFO token;

    CK_ULONG profiles[5];
};

CK_FUNCTION_LIST *provider_ctx_fns(PROVIDER_CTX *ctx);
int provider_ctx_lock_slots(PROVIDER_CTX *ctx, struct p11prov_slot **slots);
void provider_ctx_unlock_slots(PROVIDER_CTX *ctx, struct p11prov_slot **slots);

/* Debugging */
void p11prov_debug(const char *fmt, ...);
void p11prov_debug_mechanism(PROVIDER_CTX *ctx, CK_SLOT_ID slotid,
                             CK_MECHANISM_TYPE type);
void p11prov_debug_token_info(CK_TOKEN_INFO info);
void p11prov_debug_slot(struct p11prov_slot *slot);

/* Keys */
typedef struct p11prov_key P11PROV_KEY;

P11PROV_KEY *p11prov_key_ref(P11PROV_KEY *key);
void p11prov_key_free(P11PROV_KEY *key);
CK_ATTRIBUTE *p11prov_key_attr(P11PROV_KEY *key, CK_ATTRIBUTE_TYPE type);
CK_KEY_TYPE p11prov_key_type(P11PROV_KEY *key);
CK_SLOT_ID p11prov_key_slotid(P11PROV_KEY *key);
CK_OBJECT_HANDLE p11prov_key_handle(P11PROV_KEY *key);
CK_ULONG p11prov_key_size(P11PROV_KEY *key);

int find_keys(PROVIDER_CTX *provctx,
              P11PROV_KEY **priv, P11PROV_KEY **pub,
              CK_SLOT_ID slotid, CK_OBJECT_CLASS class,
              const unsigned char *id, size_t id_len,
              const char *label);

/* Object Store */
typedef struct p11prov_object P11PROV_OBJECT;

void p11prov_object_free(P11PROV_OBJECT *obj);
bool p11prov_object_check_key(P11PROV_OBJECT *obj, bool priv);
int p11prov_object_export_public_rsa_key(P11PROV_OBJECT *obj,
                                         OSSL_CALLBACK *cb_fn, void *cb_arg);
P11PROV_KEY *p11prov_object_get_key(P11PROV_OBJECT *obj, bool priv);


/* dispatching */
#define DECL_DISPATCH_FUNC(type, prefix, name) \
    static OSSL_FUNC_##type##_##name##_fn prefix##_##name

/* rsa keymgmt */
#define DISPATCH_RSAKM_FN(name) \
    DECL_DISPATCH_FUNC(keymgmt, p11prov_rsakm, name)
#define DISPATCH_RSAKM_ELEM(NAME, name) \
    { OSSL_FUNC_KEYMGMT_##NAME, (void(*)(void))p11prov_rsakm_##name }
extern const OSSL_DISPATCH p11prov_rsa_keymgmt_functions[];

/* ecdsa keymgmt */
#define DISPATCH_ECKM_FN(name) \
    DECL_DISPATCH_FUNC(keymgmt, p11prov_eckm, name)
#define DISPATCH_ECKM_ELEM(NAME, name) \
    { OSSL_FUNC_KEYMGMT_##NAME, (void(*)(void))p11prov_eckm_##name }
extern const OSSL_DISPATCH p11prov_ecdsa_keymgmt_functions[];

#define DISPATCH_STORE_FN(name) \
    DECL_DISPATCH_FUNC(store, p11prov_store, name)
#define DISPATCH_STORE_ELEM(NAME, name) \
    { OSSL_FUNC_STORE_##NAME, (void(*)(void))p11prov_store_##name }
extern const OSSL_DISPATCH p11prov_store_functions[];

/* common sig functions */
#define DISPATCH_SIG_FN(name) \
    DECL_DISPATCH_FUNC(signature, p11prov_sig, name)
#define DISPATCH_SIG_ELEM(prefix, NAME, name) \
    { OSSL_FUNC_SIGNATURE_##NAME, (void(*)(void))p11prov_##prefix##_##name }

/* rsa sig functions */
#define DISPATCH_RSASIG_FN(name) \
    DECL_DISPATCH_FUNC(signature, p11prov_rsasig, name)
extern const OSSL_DISPATCH p11prov_rsa_signature_functions[];

/* ecdsa sig functions */
#define DISPATCH_ECDSA_FN(name) \
    DECL_DISPATCH_FUNC(signature, p11prov_ecdsa, name)
extern const OSSL_DISPATCH p11prov_ecdsa_signature_functions[];

/* rsa encrypt/decrypt */
#define DISPATCH_RSAENC_FN(name) \
    DECL_DISPATCH_FUNC(asym_cipher, p11prov_rsaenc, name)
#define DISPATCH_RSAENC_ELEM(NAME, name) \
    { OSSL_FUNC_ASYM_CIPHER_##NAME, (void(*)(void))p11prov_rsaenc_##name }
extern const OSSL_DISPATCH p11prov_rsa_asym_cipher_functions[];

/* ecdh derivation */
#define DISPATCH_ECDH_FN(name) \
    DECL_DISPATCH_FUNC(keyexch, p11prov_ecdh, name)
#define DISPATCH_ECDH_ELEM(prefix, NAME, name) \
    { OSSL_FUNC_KEYEXCH_##NAME, (void(*)(void))p11prov_##prefix##_##name }
extern const OSSL_DISPATCH p11prov_ecdh_exchange_functions[];


/* Utilities to fetch objects from tokens */

struct fetch_attrs {
    CK_ATTRIBUTE_TYPE type;
    unsigned char **value;
    unsigned long *value_len;
    bool allocate;
    bool required;
};
#define FA_ASSIGN_ALL(x, _a, _b, _c, _d, _e) \
    do { \
        x.type = _a; \
        x.value = (unsigned char **)_b; \
        x.value_len = _c; \
        x.allocate = _d; \
        x.required = _e; \
    } while(0)

#define FA_RETURN_VAL(x, _a, _b) \
    do { \
        *x.value = _a; \
        *x.value_len = _b; \
    } while(0)

#define FA_RETURN_LEN(x, _a) *x.value_len = _a

#define CKATTR_ASSIGN_ALL(x, _a, _b, _c) \
    do { \
        x.type = _a; \
        x.pValue = (void *)_b; \
        x.ulValueLen = _c; \
    } while(0)

int p11prov_fetch_attributes(CK_FUNCTION_LIST *f,
                             CK_SESSION_HANDLE session,
                             CK_OBJECT_HANDLE object,
                             struct fetch_attrs *attrs,
                             unsigned long attrnums);

#endif /* _PROVIDER_H */
