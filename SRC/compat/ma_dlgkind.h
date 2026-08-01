/* ma_dlgkind.h — the template-control "kind" taxonomy, shared by the dialog-template
 * parser (ma_dlgtmpl.cpp, which assigns kinds from the "{CLSID}" class string) and its
 * consumers (afxwin.h's template-driven control hosting, ma_olecontrol.cpp).
 *
 * Kept in one header on purpose: S60 added two kinds and the values are compared across
 * TU boundaries, so a second hand-maintained copy would be a silent-drift hazard.
 *
 * Kind == the Rowan OCX coclass, identified by CLSID Data1:
 *   RSTATIC c42bac3d · RCOMBO 737cb0c9 · RLISTBOX 48814009 · RBUTTON 78918646
 *   REDIT   499e2be6 · REDTBT 461a1fe3 · RTABS   4a1e1986 · RSCRLBAR 505aee46
 */
#ifndef MA_DLGKIND_H
#define MA_DLGKIND_H

enum MaDlgKind {
    MA_K_UNKNOWN = 0,
    MA_K_RSTATIC = 1,
    MA_K_RCOMBO  = 2,
    MA_K_RLISTBOX= 3,
    MA_K_RBUTTON = 4,
    MA_K_REDIT   = 5,
    MA_K_REDTBT  = 6,
    MA_K_RTABS   = 7,   /* S60: IDJ_TABCTRL (1002) in IDD_EMPTYPAGE — the Player Log tab bar */
    MA_K_RSCRLBAR= 8    /* S60: classified for trace/audit only — NOT hosted yet (backlog) */
};

#endif /* MA_DLGKIND_H */
