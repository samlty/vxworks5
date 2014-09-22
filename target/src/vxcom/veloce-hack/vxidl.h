/* vxidl.h generated by WIDL Version 2.2.1 on 06-Dec-01 at 11:42:45 AM */

#include "comBase.h"

#ifndef __INCvxidl_h
#define __INCvxidl_h


#include "comCoreTypes.h"

#include "comAutomation.h"

#include "vxStream.h"

#include "ConnectionPoint.h"

#ifdef __cplusplus
extern "C" {
#endif

int include_vxidl (void);

#ifndef __IMarshal_FWD_DEFINED__
#define __IMarshal_FWD_DEFINED__
typedef interface IMarshal IMarshal;
#endif /* __IMarshal_FWD_DEFINED__ */

/* vxidl.h -- Copyright (c) Wind River Systems, Inc. 2001 */
typedef IMarshal* LPMARSHAL;

typedef struct
    {
    COM_VTBL_BEGIN
    COM_VTBL_ENTRY (HRESULT, QueryInterface, (IUnknown* pThis, REFIID riid, void** ppvObject));

#define IUnknown_QueryInterface(pThis, riid, ppvObject) pThis->lpVtbl->QueryInterface(COM_ADJUST_THIS(pThis), riid, ppvObject)

    COM_VTBL_ENTRY (ULONG, AddRef, (IUnknown* pThis));

#define IUnknown_AddRef(pThis) pThis->lpVtbl->AddRef(COM_ADJUST_THIS(pThis))

    COM_VTBL_ENTRY (ULONG, Release, (IUnknown* pThis));

#define IUnknown_Release(pThis) pThis->lpVtbl->Release(COM_ADJUST_THIS(pThis))

    COM_VTBL_ENTRY (HRESULT, GetUnmarshalClass, (IMarshal* pThis, REFIID riid, void* pv, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags, CLSID* pCid));

#define IMarshal_GetUnmarshalClass(pThis, riid, pv, dwDestContext, pvDestContext, mshlflags, pCid) pThis->lpVtbl->GetUnmarshalClass(COM_ADJUST_THIS(pThis), riid, pv, dwDestContext, pvDestContext, mshlflags, pCid)

    COM_VTBL_ENTRY (HRESULT, GetMarshalSizeMax, (IMarshal* pThis, REFIID riid, void* pv, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags, DWORD* pSize));

#define IMarshal_GetMarshalSizeMax(pThis, riid, pv, dwDestContext, pvDestContext, mshlflags, pSize) pThis->lpVtbl->GetMarshalSizeMax(COM_ADJUST_THIS(pThis), riid, pv, dwDestContext, pvDestContext, mshlflags, pSize)

    COM_VTBL_ENTRY (HRESULT, MarshalInterface, (IMarshal* pThis, IStream* pStm, REFIID riid, void* pv, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags));

#define IMarshal_MarshalInterface(pThis, pStm, riid, pv, dwDestContext, pvDestContext, mshlflags) pThis->lpVtbl->MarshalInterface(COM_ADJUST_THIS(pThis), pStm, riid, pv, dwDestContext, pvDestContext, mshlflags)

    COM_VTBL_ENTRY (HRESULT, UnmarshalInterface, (IMarshal* pThis, IStream* pStm, REFIID riid, void** ppv));

#define IMarshal_UnmarshalInterface(pThis, pStm, riid, ppv) pThis->lpVtbl->UnmarshalInterface(COM_ADJUST_THIS(pThis), pStm, riid, ppv)

    COM_VTBL_ENTRY (HRESULT, ReleaseMarshalData, (IMarshal* pThis, IStream* pStm));

#define IMarshal_ReleaseMarshalData(pThis, pStm) pThis->lpVtbl->ReleaseMarshalData(COM_ADJUST_THIS(pThis), pStm)

    COM_VTBL_ENTRY (HRESULT, DisconnectObject, (IMarshal* pThis, DWORD dwReserved));

#define IMarshal_DisconnectObject(pThis, dwReserved) pThis->lpVtbl->DisconnectObject(COM_ADJUST_THIS(pThis), dwReserved)

    COM_VTBL_END
    } IMarshalVtbl;

#ifdef __cplusplus

interface IMarshal : public IUnknown
{
virtual HRESULT GetUnmarshalClass (REFIID riid, void* pv, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags, CLSID* pCid) =0;

virtual HRESULT GetMarshalSizeMax (REFIID riid, void* pv, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags, DWORD* pSize) =0;

virtual HRESULT MarshalInterface (IStream* pStm, REFIID riid, void* pv, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags) =0;

virtual HRESULT UnmarshalInterface (IStream* pStm, REFIID riid, void** ppv) =0;

virtual HRESULT ReleaseMarshalData (IStream* pStm) =0;

virtual HRESULT DisconnectObject (DWORD dwReserved) =0;

};

#else

/* C interface definition for IMarshal */

interface IMarshal
    {
    const IMarshalVtbl *  lpVtbl;
    };

#endif /* __cplusplus */

EXTERN_C const IID IID_IMarshal;

#ifdef __cplusplus
}
#endif



#endif /* __INCvxidl_h */

