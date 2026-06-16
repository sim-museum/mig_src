/* ma_dlgitem.cpp — MFC dialog-data-exchange (DDX) control registry for the
 * Linux port. Real MFC creates child control HWNDs from the .rc dialog template
 * and GetDlgItem() walks them; we have no native child windows, so DDX_Control()
 * registers the C++ control wrapper object keyed by (dialog, control-id) and
 * GetDlgItem() looks it up. Gated nowhere — only linked into the MA_LINUX build. */

#pragma pack(push, 8)
#include <map>
#include <utility>
#pragma pack(pop)

extern "C" {
void  ma_ddx_register(void* dlg, int id, void* ctrl);
void* ma_ddx_lookup(void* dlg, int id);
}

static std::map<std::pair<void*, int>, void*>& ma_ddx_reg()
{
    static std::map<std::pair<void*, int>, void*> m;
    return m;
}

extern "C" void ma_ddx_register(void* dlg, int id, void* ctrl)
{
    ma_ddx_reg()[std::make_pair(dlg, id)] = ctrl;
}

extern "C" void* ma_ddx_lookup(void* dlg, int id)
{
    std::map<std::pair<void*, int>, void*>& m = ma_ddx_reg();
    std::map<std::pair<void*, int>, void*>::iterator it = m.find(std::make_pair(dlg, id));
    return it == m.end() ? (void*)0 : it->second;
}
