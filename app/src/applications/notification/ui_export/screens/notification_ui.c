#include <time.h>

#include "../notification_ui.h"

static on_notification_remove_cb_t notification_removed_callback;
static lv_obj_t *main_page;
static lv_timer_t *timer;
static active_notification_t active_notifications[ZSW_NOTIFICATION_MGR_MAX_STORED];
static uint32_t active_notification_num;

/** @brief          Convert a time in seconds to a age string.
 *  @param delta    Time in seconds
 *  @param buf      Pointer to output buffer
*/
static void notification_delta2char(uint32_t delta, char *buf)
{
    uint32_t minutes = (delta % 3600) / 60;
    uint32_t hours = (delta % 86400) / 3600;
    uint32_t days = (delta % (86400 * 30)) / 86400;

    if (minutes > 0) {
        sprintf(buf, "%u min", minutes);
    } else if (hours > 0) {
        sprintf(buf, "%u h", hours);
    } else if (days > 0) {
        sprintf(buf, "%u d", days);
    } else {
        sprintf(buf, "Now");
    }
}

/** @brief
 *  @param timer
*/
static void label_on_Timer_Callback(lv_timer_t *timer)
{
    char buf[16];
    uint32_t delta;

    for (uint32_t i = 0; i < ZSW_NOTIFICATION_MGR_MAX_STORED; i++) {
        // Make sure that the notification exists and prevent exceptions because of a fragmented array.
        if (active_notifications[i].deltaLabel != NULL) {
            delta = time(NULL) - active_notifications[i].notification->timestamp;
            notification_delta2char(delta, buf);

            lv_label_set_text(active_notifications[i].deltaLabel, buf);
        }
    }
}

/** @brief
 *  @param event
*/
static void notification_on_Clicked_Callback(lv_event_t *event)
{
    uint32_t id;

    id = (uint32_t)lv_event_get_user_data(event);

    for (uint32_t i = 0; i < ZSW_NOTIFICATION_MGR_MAX_STORED; i++) {
        if (active_notifications[i].notification->id == id) {
            notification_removed_callback(id);
            break;
        }
    }
}

/** @brief
 *  @param parent
 *  @param not
 *  @param group
*/
static void build_notification_entry(lv_obj_t *parent, zsw_not_mngr_notification_t *not, lv_group_t *group)
{
    lv_obj_t *ui_Panel;
    lv_obj_t *ui_LabelSource;
    lv_obj_t *ui_LabelTimeDelta;
    lv_obj_t *ui_ImageIcon;
    lv_obj_t *ui_LabelHeader;
    lv_obj_t *ui_LabelBody;

    const lv_img_dsc_t *image_source;
    const char *source;
    char buf[16];

    switch (not->src) {
        case NOTIFICATION_SRC_COMMON_MESSENGER:
            image_source = &ui_img_whatsapp_png;
            source = "Messenger";
            break;
        case NOTIFICATION_SRC_WHATSAPP:
            image_source = &ui_img_whatsapp_png;
            source = "WhatsApp";
            break;
        case NOTIFICATION_SRC_GMAIL:
            image_source = &ui_img_mail_png;
            source = "Gmail";
            break;
        case NOTIFICATION_SRC_COMMON_MAIL:
            image_source = &ui_img_mail_png;
            source = "Mail";
            break;
        case NOTIFICATION_SRC_HOME_ASSISTANT:
            image_source = &ui_img_homeassistant_png;
            source = "Home Assistant";
            break;
        default:
            image_source = &ui_img_gadget_png;
            source = "Unknown";
            break;
    }

    ui_Panel = lv_obj_create(parent);
    lv_obj_set_width(ui_Panel, 200);
    lv_obj_set_height(ui_Panel, 100);
    lv_obj_set_align(ui_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_bg_color(ui_Panel, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_Panel, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_Panel, notification_on_Clicked_Callback, LV_EVENT_LONG_PRESSED, (void *)not->id);

    ui_LabelSource = lv_label_create(ui_Panel);
    lv_obj_set_width(ui_LabelSource, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_LabelSource, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_LabelSource, 15);
    lv_obj_set_y(ui_LabelSource, -30);
    lv_obj_set_align(ui_LabelSource, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_LabelSource, source);
    lv_obj_clear_flag(ui_LabelSource, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_text_color(ui_LabelSource, lv_color_hex(0x8C8C8C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelSource, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelTimeDelta = lv_label_create(ui_Panel);
    lv_obj_set_width(ui_LabelTimeDelta, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_LabelTimeDelta, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_LabelTimeDelta, 70);
    lv_obj_set_y(ui_LabelTimeDelta, -30);
    lv_obj_set_align(ui_LabelTimeDelta, LV_ALIGN_CENTER);

    notification_delta2char(time(NULL) - not->timestamp, buf);
    lv_label_set_text(ui_LabelTimeDelta, buf);

    lv_obj_clear_flag(ui_LabelTimeDelta,
                      LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE |
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_text_color(ui_LabelTimeDelta, lv_color_hex(0x8C8C8C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelTimeDelta, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    active_notifications[active_notification_num].panel = ui_Panel;
    active_notifications[active_notification_num].deltaLabel = ui_LabelTimeDelta;
    active_notifications[active_notification_num].notification = not;

    ui_ImageIcon = lv_img_create(ui_Panel);
    lv_obj_set_width(ui_ImageIcon, 16);
    lv_obj_set_height(ui_ImageIcon, 16);
    lv_obj_set_x(ui_ImageIcon, -85);
    lv_obj_set_y(ui_ImageIcon, -30);
    lv_obj_set_align(ui_ImageIcon, LV_ALIGN_CENTER);
    lv_img_set_src(ui_ImageIcon, image_source);
    lv_obj_clear_flag(ui_ImageIcon, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);

    ui_LabelHeader = lv_label_create(ui_Panel);
    lv_obj_set_width(ui_LabelHeader, 180);
    lv_obj_set_height(ui_LabelHeader, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_LabelHeader, 0);
    lv_obj_set_y(ui_LabelHeader, 0);
    lv_obj_set_align(ui_LabelHeader, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelHeader, not->sender);
    lv_label_set_long_mode(ui_LabelHeader, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(ui_LabelHeader, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_text_color(ui_LabelHeader, lv_color_hex(0x587BF8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelHeader, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelHeader, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LabelBody = lv_label_create(ui_Panel);
    lv_obj_set_width(ui_LabelBody, 180);
    lv_obj_set_height(ui_LabelBody, 25);
    lv_obj_set_x(ui_LabelBody, 0);
    lv_obj_set_y(ui_LabelBody, 30);
    lv_obj_set_align(ui_LabelBody, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LabelBody, not->body);
    lv_label_set_long_mode(ui_LabelBody, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_clear_flag(ui_LabelBody, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_text_color(ui_LabelBody, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LabelBody, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LabelBody, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LabelBody, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LabelBody, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_LabelBody, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_LabelBody, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_LabelBody, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_LabelBody, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_LabelBody, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_LabelBody, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Remove the cursor and the highlighting (visible for the first entry).
    lv_obj_clear_state(ui_LabelBody, LV_STATE_CHECKED | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

    if (active_notification_num < ZSW_NOTIFICATION_MGR_MAX_STORED) {
        active_notification_num++;
    }
}

void notifications_ui_page_init(on_notification_remove_cb_t not_removed_cb)
{
    notification_removed_callback = not_removed_cb;
    active_notification_num = 0;
    memset(active_notifications, 0, sizeof(active_notifications));
}

void notifications_ui_page_create(zsw_not_mngr_notification_t *notifications, uint8_t num_notifications,
                                  lv_group_t *group)
{
    main_page = lv_obj_create(lv_scr_act());
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_size(main_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_side(main_page, LV_BORDER_SIDE_NONE, 0);
    lv_obj_center(main_page);

    lv_obj_set_flex_flow(main_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(main_page, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(main_page, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(main_page, LV_SCROLLBAR_MODE_OFF);

    /*
        ui_ImgButtonClearAll = lv_imgbtn_create(lv_obj_create(lv_scr_act()));
        lv_imgbtn_set_src(ui_ImgButtonClearAll, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_trash_png, NULL);
        lv_imgbtn_set_src(ui_ImgButtonClearAll, LV_IMGBTN_STATE_PRESSED, NULL, &ui_img_trash_png, NULL);
        lv_obj_set_width(ui_ImgButtonClearAll, 32);
        lv_obj_set_height(ui_ImgButtonClearAll, 32);
        lv_obj_align(ui_ImgButtonClearAll, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_align(ui_ImgButtonClearAll, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_ImgButtonClearAll, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_O
        BJ_FLAG_SNAPPABLE);
        lv_obj_add_event_cb(ui_ImgButtonClearAll, on_ImgButtonClearAll_clicked, LV_EVENT_PRESSED, NULL);
    */

    for (int i = 0; i < num_notifications; i++) {
        build_notification_entry(main_page, &notifications[i], group);
    }

    // Update the notifications position manually firt time.
    lv_event_send(main_page, LV_EVENT_SCROLL, NULL);

    // Be sure the fist notification is in the middle.
    lv_obj_scroll_to_view(lv_obj_get_child(main_page, 0), LV_ANIM_OFF);

    timer = lv_timer_create(label_on_Timer_Callback, 5000UL, NULL);
}

void notifications_ui_page_close(void)
{
    if (timer != NULL) {
        lv_timer_del(timer);
    }

    lv_obj_del(main_page);
    main_page = NULL;
    timer = NULL;
}

void notifications_ui_add_notification(zsw_not_mngr_notification_t *not, lv_group_t *group)
{
    if (main_page == NULL) {
        return;
    }

    build_notification_entry(main_page, not, group);
    lv_obj_scroll_to_view(lv_obj_get_child(main_page, -1), LV_ANIM_OFF);
    lv_obj_update_layout(main_page);
}

void notifications_ui_remove_notification(uint32_t id)
{
    if (main_page == NULL) {
        return;
    }

    for (uint32_t i = 0; i < ZSW_NOTIFICATION_MGR_MAX_STORED; i++) {
        if (active_notifications[i].notification->id == id) {
            lv_obj_del(active_notifications[i].panel);
            lv_obj_scroll_to_view(lv_obj_get_child(main_page, -1), LV_ANIM_ON);
            lv_obj_update_layout(main_page);

            active_notification_num--;

            break;
        }
    }
}