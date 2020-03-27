#include "wxExtensions.hpp"

#include <stdexcept>
#include <cmath>

#include <wx/sizer.h>

#include <boost/algorithm/string/replace.hpp>

#include "BitmapCache.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "GUI_ObjectList.hpp"
#include "libslic3r/Config.hpp"
#include "I18N.hpp"
#include "GUI_Utils.hpp"
#include "../Utils/MacDarkMode.hpp"

#ifndef __WXGTK__// msw_menuitem_bitmaps is used for MSW and OSX
static std::map<int, std::string> msw_menuitem_bitmaps;
#ifdef __WXMSW__
void msw_rescale_menu(wxMenu* menu)
{
	struct update_icons {
		static void run(wxMenuItem* item) {
			const auto it = msw_menuitem_bitmaps.find(item->GetId());
			if (it != msw_menuitem_bitmaps.end()) {
				const wxBitmap& item_icon = create_scaled_bitmap(it->second);
				if (item_icon.IsOk())
					item->SetBitmap(item_icon);
			}
			if (item->IsSubMenu())
				for (wxMenuItem *sub_item : item->GetSubMenu()->GetMenuItems())
					update_icons::run(sub_item);
		}
	};

	for (wxMenuItem *item : menu->GetMenuItems())
		update_icons::run(item);
}
#endif /* __WXMSW__ */
#endif /* no __WXGTK__ */

void enable_menu_item(wxUpdateUIEvent& evt, std::function<bool()> const cb_condition, wxMenuItem* item, wxWindow* win)
{
    const bool enable = cb_condition();
    evt.Enable(enable);

#ifdef __WXOSX__
    const auto it = msw_menuitem_bitmaps.find(item->GetId());
    if (it != msw_menuitem_bitmaps.end())
    {
        const wxBitmap& item_icon = create_scaled_bitmap(it->second, win, 16, !enable);
        if (item_icon.IsOk())
            item->SetBitmap(item_icon);
    }
#endif // __WXOSX__
}

wxMenuItem* append_menu_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent& event)> cb, const wxBitmap& icon, wxEvtHandler* event_handler,
    std::function<bool()> const cb_condition, wxWindow* parent)
{
    if (id == wxID_ANY)
        id = wxNewId();

    auto *item = new wxMenuItem(menu, id, string, description);
    if (icon.IsOk()) {
        item->SetBitmap(icon);
    }
    menu->Append(item);

#ifdef __WXMSW__
    if (event_handler != nullptr && event_handler != menu)
        event_handler->Bind(wxEVT_MENU, cb, id);
    else
#endif // __WXMSW__
        menu->Bind(wxEVT_MENU, cb, id);

    if (parent) {
        parent->Bind(wxEVT_UPDATE_UI, [cb_condition, item, parent](wxUpdateUIEvent& evt) {
            enable_menu_item(evt, cb_condition, item, parent); }, id);
    }

    return item;
}

wxMenuItem* append_menu_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent& event)> cb, const std::string& icon, wxEvtHandler* event_handler,
    std::function<bool()> const cb_condition, wxWindow* parent)
{
    if (id == wxID_ANY)
        id = wxNewId();

    const wxBitmap& bmp = !icon.empty() ? create_scaled_bitmap(icon) : wxNullBitmap;   // FIXME: pass window ptr
//#ifdef __WXMSW__
#ifndef __WXGTK__
    if (bmp.IsOk())
        msw_menuitem_bitmaps[id] = icon;
#endif /* __WXMSW__ */

    return append_menu_item(menu, id, string, description, cb, bmp, event_handler, cb_condition, parent);
}

wxMenuItem* append_submenu(wxMenu* menu, wxMenu* sub_menu, int id, const wxString& string, const wxString& description, const std::string& icon,
    std::function<bool()> const cb_condition, wxWindow* parent)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxMenuItem* item = new wxMenuItem(menu, id, string, description);
    if (!icon.empty()) {
        item->SetBitmap(create_scaled_bitmap(icon));    // FIXME: pass window ptr
//#ifdef __WXMSW__
#ifndef __WXGTK__
        msw_menuitem_bitmaps[id] = icon;
#endif /* __WXMSW__ */
    }

    item->SetSubMenu(sub_menu);
    menu->Append(item);

    if (parent) {
        parent->Bind(wxEVT_UPDATE_UI, [cb_condition, item, parent](wxUpdateUIEvent& evt) {
            enable_menu_item(evt, cb_condition, item, parent); }, id);
    }

    return item;
}

wxMenuItem* append_menu_radio_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent& event)> cb, wxEvtHandler* event_handler)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxMenuItem* item = menu->AppendRadioItem(id, string, description);

#ifdef __WXMSW__
    if (event_handler != nullptr && event_handler != menu)
        event_handler->Bind(wxEVT_MENU, cb, id);
    else
#endif // __WXMSW__
        menu->Bind(wxEVT_MENU, cb, id);

    return item;
}

wxMenuItem* append_menu_check_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent & event)> cb, wxEvtHandler* event_handler,
    std::function<bool()> const enable_condition, std::function<bool()> const check_condition, wxWindow* parent)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxMenuItem* item = menu->AppendCheckItem(id, string, description);

#ifdef __WXMSW__
    if (event_handler != nullptr && event_handler != menu)
        event_handler->Bind(wxEVT_MENU, cb, id);
    else
#endif // __WXMSW__
        menu->Bind(wxEVT_MENU, cb, id);

    if (parent)
        parent->Bind(wxEVT_UPDATE_UI, [enable_condition, check_condition](wxUpdateUIEvent& evt)
            {
                evt.Enable(enable_condition());
                evt.Check(check_condition());
            }, id);

    return item;
}

const unsigned int wxCheckListBoxComboPopup::DefaultWidth = 200;
const unsigned int wxCheckListBoxComboPopup::DefaultHeight = 200;
const unsigned int wxCheckListBoxComboPopup::DefaultItemHeight = 18;

bool wxCheckListBoxComboPopup::Create(wxWindow* parent)
{
    return wxCheckListBox::Create(parent, wxID_HIGHEST + 1, wxPoint(0, 0));
}

wxWindow* wxCheckListBoxComboPopup::GetControl()
{
    return this;
}

void wxCheckListBoxComboPopup::SetStringValue(const wxString& value)
{
    m_text = value;
}

wxString wxCheckListBoxComboPopup::GetStringValue() const
{
    return m_text;
}

wxSize wxCheckListBoxComboPopup::GetAdjustedSize(int minWidth, int prefHeight, int maxHeight)
{
    // matches owner wxComboCtrl's width
    // and sets height dinamically in dependence of contained items count

    wxComboCtrl* cmb = GetComboCtrl();
    if (cmb != nullptr)
    {
        wxSize size = GetComboCtrl()->GetSize();

        unsigned int count = GetCount();
        if (count > 0)
            size.SetHeight(count * DefaultItemHeight);
        else
            size.SetHeight(DefaultHeight);

        return size;
    }
    else
        return wxSize(DefaultWidth, DefaultHeight);
}

void wxCheckListBoxComboPopup::OnKeyEvent(wxKeyEvent& evt)
{
    // filters out all the keys which are not working properly
    switch (evt.GetKeyCode())
    {
    case WXK_LEFT:
    case WXK_UP:
    case WXK_RIGHT:
    case WXK_DOWN:
    case WXK_PAGEUP:
    case WXK_PAGEDOWN:
    case WXK_END:
    case WXK_HOME:
    case WXK_NUMPAD_LEFT:
    case WXK_NUMPAD_UP:
    case WXK_NUMPAD_RIGHT:
    case WXK_NUMPAD_DOWN:
    case WXK_NUMPAD_PAGEUP:
    case WXK_NUMPAD_PAGEDOWN:
    case WXK_NUMPAD_END:
    case WXK_NUMPAD_HOME:
    {
        break;
    }
    default:
    {
        evt.Skip();
        break;
    }
    }
}

void wxCheckListBoxComboPopup::OnCheckListBox(wxCommandEvent& evt)
{
    // forwards the checklistbox event to the owner wxComboCtrl

    if (m_check_box_events_status == OnCheckListBoxFunction::FreeToProceed )
    {
        wxComboCtrl* cmb = GetComboCtrl();
        if (cmb != nullptr) {
            wxCommandEvent event(wxEVT_CHECKLISTBOX, cmb->GetId());
            event.SetEventObject(cmb);
            cmb->ProcessWindowEvent(event);
        }
    }

    evt.Skip();

    #ifndef _WIN32  // events are sent differently on OSX+Linux vs Win (more description in header file)
        if ( m_check_box_events_status == OnCheckListBoxFunction::RefuseToProceed )
            // this happens if the event was resent by OnListBoxSelection - next call to OnListBoxSelection is due to user clicking the text, so the function should
            // explicitly change the state on the checkbox
            m_check_box_events_status = OnCheckListBoxFunction::WasRefusedLastTime;
        else
            // if the user clicked the checkbox square, this event was sent before OnListBoxSelection was called, so we don't want it to resend it
            m_check_box_events_status = OnCheckListBoxFunction::RefuseToProceed;
    #endif
}

void wxCheckListBoxComboPopup::OnListBoxSelection(wxCommandEvent& evt)
{
    // transforms list box item selection event into checklistbox item toggle event 

    int selId = GetSelection();
    if (selId != wxNOT_FOUND)
    {
        #ifndef _WIN32
            if (m_check_box_events_status == OnCheckListBoxFunction::RefuseToProceed)
        #endif
                Check((unsigned int)selId, !IsChecked((unsigned int)selId));

        m_check_box_events_status = OnCheckListBoxFunction::FreeToProceed; // so the checkbox reacts to square-click the next time

        SetSelection(wxNOT_FOUND);
        wxCommandEvent event(wxEVT_CHECKLISTBOX, GetId());
        event.SetInt(selId);
        event.SetEventObject(this);
        ProcessEvent(event);
    }
}


namespace Slic3r {
namespace GUI {

// ***  PresetBitmapComboBox  ***

/* For PresetBitmapComboBox we use bitmaps that are created from images that are already scaled appropriately for Retina
 * (Contrary to the intuition, the `scale` argument for Bitmap's constructor doesn't mean
 * "please scale this to such and such" but rather
 * "the wxImage is already sized for backing scale such and such". )
 * Unfortunately, the constructor changes the size of wxBitmap too.
 * Thus We need to use unscaled size value for bitmaps that we use
 * to avoid scaled size of control items.
 * For this purpose control drawing methods and
 * control size calculation methods (virtual) are overridden.
 **/

PresetBitmapComboBox::PresetBitmapComboBox(wxWindow* parent, const wxSize& size) :
    wxBitmapComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size, 0, nullptr, wxCB_READONLY)
{}

#ifdef __APPLE__
bool PresetBitmapComboBox::OnAddBitmap(const wxBitmap& bitmap)
{
    if (bitmap.IsOk())
    {
        // we should use scaled! size values of bitmap
        int width = (int)bitmap.GetScaledWidth();
        int height = (int)bitmap.GetScaledHeight();

        if (m_usedImgSize.x < 0)
        {
            // If size not yet determined, get it from this image.
            m_usedImgSize.x = width;
            m_usedImgSize.y = height;

            // Adjust control size to vertically fit the bitmap
            wxWindow* ctrl = GetControl();
            ctrl->InvalidateBestSize();
            wxSize newSz = ctrl->GetBestSize();
            wxSize sz = ctrl->GetSize();
            if (newSz.y > sz.y)
                ctrl->SetSize(sz.x, newSz.y);
            else
                DetermineIndent();
        }

        wxCHECK_MSG(width == m_usedImgSize.x && height == m_usedImgSize.y,
            false,
            "you can only add images of same size");

        return true;
    }

    return false;
}

void PresetBitmapComboBox::OnDrawItem(wxDC& dc,
    const wxRect& rect,
    int item,
    int flags) const
{
    const wxBitmap& bmp = *(wxBitmap*)m_bitmaps[item];
    if (bmp.IsOk())
    {
        // we should use scaled! size values of bitmap
        wxCoord w = bmp.GetScaledWidth();
        wxCoord h = bmp.GetScaledHeight();

        const int imgSpacingLeft = 4;

        // Draw the image centered
        dc.DrawBitmap(bmp,
            rect.x + (m_usedImgSize.x - w) / 2 + imgSpacingLeft,
            rect.y + (rect.height - h) / 2,
            true);
    }

    wxString text = GetString(item);
    if (!text.empty())
        dc.DrawText(text,
            rect.x + m_imgAreaWidth + 1,
            rect.y + (rect.height - dc.GetCharHeight()) / 2);
}
#endif
}
}


// ***  wxDataViewTreeCtrlComboPopup  ***

const unsigned int wxDataViewTreeCtrlComboPopup::DefaultWidth = 270;
const unsigned int wxDataViewTreeCtrlComboPopup::DefaultHeight = 200;
const unsigned int wxDataViewTreeCtrlComboPopup::DefaultItemHeight = 22;

bool wxDataViewTreeCtrlComboPopup::Create(wxWindow* parent)
{
	return wxDataViewTreeCtrl::Create(parent, wxID_ANY/*HIGHEST + 1*/, wxPoint(0, 0), wxDefaultSize/*wxSize(270, -1)*/, wxDV_NO_HEADER);
}
/*
wxSize wxDataViewTreeCtrlComboPopup::GetAdjustedSize(int minWidth, int prefHeight, int maxHeight)
{
	// matches owner wxComboCtrl's width
	// and sets height dinamically in dependence of contained items count
	wxComboCtrl* cmb = GetComboCtrl();
	if (cmb != nullptr)
	{
		wxSize size = GetComboCtrl()->GetSize();
		if (m_cnt_open_items > 0)
			size.SetHeight(m_cnt_open_items * DefaultItemHeight);
		else
			size.SetHeight(DefaultHeight);

		return size;
	}
	else
		return wxSize(DefaultWidth, DefaultHeight);
}
*/
void wxDataViewTreeCtrlComboPopup::OnKeyEvent(wxKeyEvent& evt)
{
	// filters out all the keys which are not working properly
	if (evt.GetKeyCode() == WXK_UP)
	{
		return;
	}
	else if (evt.GetKeyCode() == WXK_DOWN)
	{
		return;
	}
	else
	{
		evt.Skip();
		return;
	}
}

void wxDataViewTreeCtrlComboPopup::OnDataViewTreeCtrlSelection(wxCommandEvent& evt)
{
	wxComboCtrl* cmb = GetComboCtrl();
	auto selected = GetItemText(GetSelection());
	cmb->SetText(selected);
}

// edit tooltip : change Slic3r to SLIC3R_APP_KEY
// Temporary workaround for localization
void edit_tooltip(wxString& tooltip)
{
    tooltip.Replace("Slic3r", SLIC3R_APP_KEY, true);
}

/* Function for rescale of buttons in Dialog under MSW if dpi is changed.
 * btn_ids - vector of buttons identifiers
 */
void msw_buttons_rescale(wxDialog* dlg, const int em_unit, const std::vector<int>& btn_ids)
{
    const wxSize& btn_size = wxSize(-1, int(2.5f * em_unit + 0.5f));

    for (int btn_id : btn_ids) {
        // There is a case [FirmwareDialog], when we have wxControl instead of wxButton
        // so let casting everything to the wxControl
        wxControl* btn = static_cast<wxControl*>(dlg->FindWindowById(btn_id, dlg));
        if (btn)
            btn->SetMinSize(btn_size);
    }
}

/* Function for getting of em_unit value from correct parent.
 * In most of cases it is m_em_unit value from GUI_App,
 * but for DPIDialogs it's its own value. 
 * This value will be used to correct rescale after moving between 
 * Displays with different HDPI */
int em_unit(wxWindow* win)
{
    if (win)
    {
        wxTopLevelWindow *toplevel = Slic3r::GUI::find_toplevel_parent(win);
        Slic3r::GUI::DPIDialog* dlg = dynamic_cast<Slic3r::GUI::DPIDialog*>(toplevel);
        if (dlg)
            return dlg->em_unit();
        Slic3r::GUI::DPIFrame* frame = dynamic_cast<Slic3r::GUI::DPIFrame*>(toplevel);
        if (frame)
            return frame->em_unit();
    }
    
    return Slic3r::GUI::wxGetApp().em_unit();
}

int mode_icon_px_size()
{
#ifdef __APPLE__
    return 10;
#else
    return 12;
#endif
}

// win is used to get a correct em_unit value
// It's important for bitmaps of dialogs.
// if win == nullptr, em_unit value of MainFrame will be used
wxBitmap create_scaled_bitmap(  const std::string& bmp_name_in, 
                                wxWindow *win/* = nullptr*/,
                                const int px_cnt/* = 16*/, 
                                const bool grayscale/* = false*/)
{
    static Slic3r::GUI::BitmapCache cache;

    unsigned int width = 0;
    unsigned int height = (unsigned int)(em_unit(win) * px_cnt * 0.1f + 0.5f);

    std::string bmp_name = bmp_name_in;
    boost::replace_last(bmp_name, ".png", "");

    // Try loading an SVG first, then PNG if SVG is not found:
    wxBitmap *bmp = cache.load_svg(bmp_name, width, height, grayscale, Slic3r::GUI::wxGetApp().dark_mode());
    if (bmp == nullptr) {
        bmp = cache.load_png(bmp_name, width, height, grayscale);
    }

    if (bmp == nullptr) {
        // Neither SVG nor PNG has been found, raise error
        throw std::runtime_error("Could not load bitmap: " + bmp_name);
    }

    return *bmp;
}

std::vector<wxBitmap*> get_extruder_color_icons(bool thin_icon/* = false*/)
{
    static Slic3r::GUI::BitmapCache bmp_cache;

    // Create the bitmap with color bars.
    std::vector<wxBitmap*> bmps;
    std::vector<std::string> colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();

    if (colors.empty())
        return bmps;

    unsigned char rgb[3];

    /* It's supposed that standard size of an icon is 36px*16px for 100% scaled display.
     * So set sizes for solid_colored icons used for filament preset
     * and scale them in respect to em_unit value
     */
    const double em = Slic3r::GUI::wxGetApp().em_unit();
    const int icon_width = lround((thin_icon ? 1.6 : 3.2) * em);
    const int icon_height = lround(1.6 * em);

    for (const std::string& color : colors)
    {
        std::string bitmap_key = color + "-h" + std::to_string(icon_height) + "-w" + std::to_string(icon_width);

        wxBitmap* bitmap = bmp_cache.find(bitmap_key);
        if (bitmap == nullptr) {
            // Paint the color icon.
            Slic3r::GUI::BitmapCache::parse_color(color, rgb);
            // there is no neede to scale created solid bitmap
            bitmap = bmp_cache.insert(bitmap_key, bmp_cache.mksolid(icon_width, icon_height, rgb, true));
        }
        bmps.emplace_back(bitmap);
    }

    return bmps;
}


void apply_extruder_selector(wxBitmapComboBox** ctrl, 
                             wxWindow* parent,
                             const std::string& first_item/* = ""*/, 
                             wxPoint pos/* = wxDefaultPosition*/,
                             wxSize size/* = wxDefaultSize*/,
                             bool use_thin_icon/* = false*/)
{
    std::vector<wxBitmap*> icons = get_extruder_color_icons(use_thin_icon);

    if (!*ctrl)
        *ctrl = new wxBitmapComboBox(parent, wxID_ANY, wxEmptyString, pos, size,
            0, nullptr, wxCB_READONLY);
    else
    {
        (*ctrl)->SetPosition(pos);
        (*ctrl)->SetMinSize(size);
        (*ctrl)->SetSize(size);
        (*ctrl)->Clear();
    }
    if (first_item.empty())
        (*ctrl)->Hide();    // to avoid unwanted rendering before layout (ExtruderSequenceDialog)

    if (icons.empty() && !first_item.empty()) {
        (*ctrl)->Append(_(first_item), wxNullBitmap);
        return;
    }

    // For ObjectList we use short extruder name (just a number)
    const bool use_full_item_name = dynamic_cast<Slic3r::GUI::ObjectList*>(parent) == nullptr;

    int i = 0;
    wxString str = _(L("Extruder"));
    for (wxBitmap* bmp : icons) {
        if (i == 0) {
            if (!first_item.empty())
                (*ctrl)->Append(_(first_item), *bmp);
            ++i;
        }

        (*ctrl)->Append(use_full_item_name
                        ? Slic3r::GUI::from_u8((boost::format("%1% %2%") % str % i).str())
                        : wxString::Format("%d", i), *bmp);
        ++i;
    }
    (*ctrl)->SetSelection(0);
}


// ----------------------------------------------------------------------------
// LockButton
// ----------------------------------------------------------------------------

LockButton::LockButton( wxWindow *parent, 
                        wxWindowID id, 
                        const wxPoint& pos /*= wxDefaultPosition*/, 
                        const wxSize& size /*= wxDefaultSize*/):
                        wxButton(parent, id, wxEmptyString, pos, size, wxBU_EXACTFIT | wxNO_BORDER)
{
    m_bmp_lock_closed   = ScalableBitmap(this, "lock_closed");
    m_bmp_lock_closed_f = ScalableBitmap(this, "lock_closed_f");
    m_bmp_lock_open     = ScalableBitmap(this, "lock_open");
    m_bmp_lock_open_f   = ScalableBitmap(this, "lock_open_f");

#ifdef __WXMSW__
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif // __WXMSW__
    SetBitmap(m_bmp_lock_open.bmp());
    SetBitmapDisabled(m_bmp_lock_open.bmp());
    SetBitmapHover(m_bmp_lock_closed_f.bmp());

    //button events
    Bind(wxEVT_BUTTON, &LockButton::OnButton, this);
}

void LockButton::OnButton(wxCommandEvent& event)
{
    if (m_disabled)
        return;

    m_is_pushed = !m_is_pushed;
    update_button_bitmaps();

    event.Skip();
}

void LockButton::SetLock(bool lock)
{
    m_is_pushed = lock;
    update_button_bitmaps();
}

void LockButton::msw_rescale()
{
    m_bmp_lock_closed.msw_rescale();
    m_bmp_lock_closed_f.msw_rescale();
    m_bmp_lock_open.msw_rescale();
    m_bmp_lock_open_f.msw_rescale();

    update_button_bitmaps();
}

void LockButton::update_button_bitmaps()
{
    SetBitmap(m_is_pushed ? m_bmp_lock_closed.bmp() : m_bmp_lock_open.bmp());
    SetBitmapHover(m_is_pushed ? m_bmp_lock_closed_f.bmp() : m_bmp_lock_open_f.bmp());

    Refresh();
    Update();
}



// ----------------------------------------------------------------------------
// ModeButton
// ----------------------------------------------------------------------------

ModeButton::ModeButton( wxWindow *          parent,
                        wxWindowID          id,
                        const std::string&  icon_name   /* = ""*/,
                        const wxString&     mode        /* = wxEmptyString*/,
                        const wxSize&       size        /* = wxDefaultSize*/,
                        const wxPoint&      pos         /* = wxDefaultPosition*/) :
    ScalableButton(parent, id, icon_name, mode, size, pos, wxBU_EXACTFIT)
{
    Init(mode);
}

ModeButton::ModeButton( wxWindow*           parent,
                        const wxString&     mode/* = wxEmptyString*/,
                        const std::string&  icon_name/* = ""*/,
                        int                 px_cnt/* = 16*/) :
    ScalableButton(parent, wxID_ANY, ScalableBitmap(parent, icon_name, px_cnt), mode, wxBU_EXACTFIT)
{
    Init(mode);
}

void ModeButton::Init(const wxString &mode)
{
    std::string mode_str = std::string(mode.ToUTF8());
    m_tt_focused  = Slic3r::GUI::from_u8((boost::format(_utf8(L("Switch to the %s mode"))) % mode_str).str());
    m_tt_selected = Slic3r::GUI::from_u8((boost::format(_utf8(L("Current mode is %s"))) % mode_str).str());

    SetBitmapMargins(3, 0);

    //button events
    Bind(wxEVT_BUTTON,          &ModeButton::OnButton, this);
    Bind(wxEVT_ENTER_WINDOW,    &ModeButton::OnEnterBtn, this);
    Bind(wxEVT_LEAVE_WINDOW,    &ModeButton::OnLeaveBtn, this);
}

void ModeButton::OnButton(wxCommandEvent& event)
{
    m_is_selected = true;
    focus_button(m_is_selected);

    event.Skip();
}

void ModeButton::SetState(const bool state)
{
    m_is_selected = state;
    focus_button(m_is_selected);
    SetToolTip(state ? m_tt_selected : m_tt_focused);
}

void ModeButton::focus_button(const bool focus)
{
    const wxFont& new_font = focus ? 
                             Slic3r::GUI::wxGetApp().bold_font() : 
                             Slic3r::GUI::wxGetApp().normal_font();

    SetFont(new_font);
    SetForegroundColour(wxSystemSettings::GetColour(focus ? wxSYS_COLOUR_BTNTEXT : wxSYS_COLOUR_BTNSHADOW));

    Refresh();
    Update();
}


// ----------------------------------------------------------------------------
// ModeSizer
// ----------------------------------------------------------------------------

ModeSizer::ModeSizer(wxWindow *parent, int hgap/* = 0*/) :
    wxFlexGridSizer(3, 0, hgap)
{
    SetFlexibleDirection(wxHORIZONTAL);

    std::vector < std::pair < wxString, std::string >> buttons = {
        {_(L("Simple")),    "mode_simple"},
        {_(L("Advanced")),  "mode_advanced"},
        {_(L("Expert")),    "mode_expert"},
    };

    auto modebtnfn = [](wxCommandEvent &event, int mode_id) {
        Slic3r::GUI::wxGetApp().save_mode(mode_id);
        event.Skip();
    };
    
    m_mode_btns.reserve(3);
    for (const auto& button : buttons) {
        m_mode_btns.push_back(new ModeButton(parent, button.first, button.second, mode_icon_px_size()));

        m_mode_btns.back()->Bind(wxEVT_BUTTON, std::bind(modebtnfn, std::placeholders::_1, int(m_mode_btns.size() - 1)));
        Add(m_mode_btns.back());
    }
}

void ModeSizer::SetMode(const int mode)
{
    for (size_t m = 0; m < m_mode_btns.size(); m++)
        m_mode_btns[m]->SetState(int(m) == mode);
}


void ModeSizer::msw_rescale()
{
    for (size_t m = 0; m < m_mode_btns.size(); m++)
        m_mode_btns[m]->msw_rescale();
}

// ----------------------------------------------------------------------------
// MenuWithSeparators
// ----------------------------------------------------------------------------

void MenuWithSeparators::DestroySeparators()
{
    if (m_separator_frst) {
        Destroy(m_separator_frst);
        m_separator_frst = nullptr;
    }

    if (m_separator_scnd) {
        Destroy(m_separator_scnd);
        m_separator_scnd = nullptr;
    }
}

void MenuWithSeparators::SetFirstSeparator()
{
    m_separator_frst = this->AppendSeparator();
}

void MenuWithSeparators::SetSecondSeparator()
{
    m_separator_scnd = this->AppendSeparator();
}

// ----------------------------------------------------------------------------
// PrusaBitmap
// ----------------------------------------------------------------------------
ScalableBitmap::ScalableBitmap( wxWindow *parent, 
                                const std::string& icon_name/* = ""*/,
                                const int px_cnt/* = 16*/):
    m_parent(parent), m_icon_name(icon_name),
    m_px_cnt(px_cnt)
{
    m_bmp = create_scaled_bitmap(icon_name, parent, px_cnt);
}

wxSize ScalableBitmap::GetBmpSize() const
{
#ifdef __APPLE__
    return m_bmp.GetScaledSize();
#else
    return m_bmp.GetSize();
#endif
}

int ScalableBitmap::GetBmpWidth() const
{
#ifdef __APPLE__
    return m_bmp.GetScaledWidth();
#else
    return m_bmp.GetWidth();
#endif
}

int ScalableBitmap::GetBmpHeight() const
{
#ifdef __APPLE__
    return m_bmp.GetScaledHeight();
#else
    return m_bmp.GetHeight();
#endif
}


void ScalableBitmap::msw_rescale()
{
    m_bmp = create_scaled_bitmap(m_icon_name, m_parent, m_px_cnt);
}

// ----------------------------------------------------------------------------
// PrusaButton
// ----------------------------------------------------------------------------

ScalableButton::ScalableButton( wxWindow *          parent,
                                wxWindowID          id,
                                const std::string&  icon_name /*= ""*/,
                                const wxString&     label /* = wxEmptyString*/,
                                const wxSize&       size /* = wxDefaultSize*/,
                                const wxPoint&      pos /* = wxDefaultPosition*/,
                                long                style /*= wxBU_EXACTFIT | wxNO_BORDER*/) :
    m_current_icon_name(icon_name),
    m_parent(parent)
{
    Create(parent, id, label, pos, size, style);
#ifdef __WXMSW__
    if (style & wxNO_BORDER)
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif // __WXMSW__

    SetBitmap(create_scaled_bitmap(icon_name, parent));

    if (size != wxDefaultSize)
    {
        const int em = em_unit(parent);
        m_width = size.x/em;
        m_height= size.y/em;
    }
}


ScalableButton::ScalableButton( wxWindow *          parent, 
                                wxWindowID          id,
                                const ScalableBitmap&  bitmap,
                                const wxString&     label /*= wxEmptyString*/, 
                                long                style /*= wxBU_EXACTFIT | wxNO_BORDER*/) :
    m_parent(parent),
    m_current_icon_name(bitmap.name()),
    m_px_cnt(bitmap.px_cnt())
{
    Create(parent, id, label, wxDefaultPosition, wxDefaultSize, style);
#ifdef __WXMSW__
    if (style & wxNO_BORDER)
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif // __WXMSW__

    SetBitmap(bitmap.bmp());
}

void ScalableButton::SetBitmap_(const ScalableBitmap& bmp)
{
    SetBitmap(bmp.bmp());
    m_current_icon_name = bmp.name();
}

void ScalableButton::SetBitmapDisabled_(const ScalableBitmap& bmp)
{
    SetBitmapDisabled(bmp.bmp());
    m_disabled_icon_name = bmp.name();
}

int ScalableButton::GetBitmapHeight()
{
#ifdef __APPLE__
    return GetBitmap().GetScaledHeight();
#else
    return GetBitmap().GetHeight();
#endif
}

void ScalableButton::msw_rescale()
{
    SetBitmap(create_scaled_bitmap(m_current_icon_name, m_parent, m_px_cnt));
    if (!m_disabled_icon_name.empty())
        SetBitmapDisabled(create_scaled_bitmap(m_disabled_icon_name, m_parent, m_px_cnt));

    if (m_width > 0 || m_height>0)
    {
        const int em = em_unit(m_parent);
        wxSize size(m_width * em, m_height * em);
        SetMinSize(size);
    }
}



