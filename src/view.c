/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018 ZondaX GmbH
*   (c) 2019 Binance
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "view.h"
#include "view_templates.h"
#include "view_expl.h"
#include "view_conf.h"
#include "common.h"
#include "app_main.h"

#include "glyphs.h"
#include "bagl.h"

#include <string.h>
#include <stdio.h>

#define TRUE  1
#define FALSE 0

#ifdef TARGET_NANOX
#include "ux.h"
    ux_state_t G_ux;
    bolos_ux_params_t G_ux_params;
#else // TARGET_NANOX
    ux_state_t ux;
#endif // TARGET_NANOX
enum UI_STATE view_uiState;

void reject(unsigned int unused);

//------ View elements
const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_address[];
const ux_menu_entry_t menu_about[];

const ux_menu_entry_t menu_transaction_info[] = {
        {NULL, view_sign_transaction, 0, NULL, "Sign transaction", NULL, 0, 0},
        {NULL, view_tx_show, 0, NULL, "Preview again", NULL, 0, 0},
        {NULL, reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
        UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
#ifdef TESTING_ENABLED
    {NULL, NULL, 0, &C_icon_app, "Binance Chain", "TEST MODE", 31, 8},
#else
    {NULL, NULL, 0, &C_icon_app, "Binance Chain", "Ready", 31, 8},
#endif
    {menu_address, NULL, 0, NULL, "Your addresses", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};

const ux_menu_entry_t menu_address[] = {
    {NULL, view_address_show_main_net, 0, NULL, "Main net", NULL, 0, 0},
    {NULL, view_address_show_test_net, 0, NULL, "Test net", NULL, 0, 0},
    {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};

const ux_menu_entry_t menu_about[] = {
        {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
        {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
        UX_MENU_END
};
//------ View elements

//------ Event handlers
viewctl_delegate_getData ehGetData = NULL;
viewctl_delegate_accept ehAccept = NULL;
viewctl_delegate_reject ehReject = NULL;

void view_set_handlers(viewctl_delegate_getData func_getData,
                       viewctl_delegate_accept func_accept,
                       viewctl_delegate_reject func_reject) {
    ehGetData = func_getData;
    ehAccept = func_accept;
    ehReject = func_reject;
}

// ------ Event handlers

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

/////////////////////////////////

void user_view_addr_exit(unsigned int unused) {
    view_idle(0);
}

void view_address_show_main_net(unsigned int unused) {
    UNUSED(unused);

    set_hrp("bnb");
    view_set_handlers(addr_getData, NULL, NULL);
    viewexpl_start(0,
                   false,
                   ehGetData,   // update
                   NULL,        // ready
                   user_view_addr_exit // exit
    );
}

void view_address_show_test_net(unsigned int unused) {
    UNUSED(unused);

    set_hrp("tbnb");
    view_set_handlers(addr_getData, NULL, NULL);
    viewexpl_start(0,
                   false,
                   ehGetData,   // update
                   NULL,        // ready
                   user_view_addr_exit // exit
    );
}

/////////////////////////////////

void view_tx_show(unsigned int start_page) {
    if (ehGetData == NULL) { return; }

    viewexpl_start(start_page,
                   false,         // multi-page
                   ehGetData,
                   NULL,
                   view_display_tx_menu);
}

void view_addr_exit(unsigned int unused) {
    G_io_apdu_buffer[0] = 0x90;
    G_io_apdu_buffer[1] = 0x00;
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

void view_addr_show(unsigned int start_page) {
    viewexpl_start(start_page,
                   true,          // single page
                   ehGetData,     // update
                   NULL,          // ready
                   view_addr_exit // exit
    );
}

void view_addr_confirm(unsigned int start_page) {
    viewconf_start(start_page,
                   true,        // single page
                   ehGetData,   // update
                   NULL,        // ready
                   NULL,        // exit
                   ehAccept,
                   ehReject);
}

/////////////////////////////////

void view_sign_transaction(unsigned int unused) {
    UNUSED(unused);
    if (ehAccept != NULL) {
        ehAccept();
    }
}

void reject(unsigned int unused) {
    if (ehReject != NULL) {
        ehReject();
    }
}

void view_init(void) {
    UX_INIT();
    view_uiState = UI_IDLE;
}

void view_idle(unsigned int ignored) {
    view_uiState = UI_IDLE;
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

void view_display_tx_menu(unsigned int ignored) {
    view_uiState = UI_TRANSACTION;
    UX_MENU_DISPLAY(0, menu_transaction_info, NULL);
}

void view_display_signing_success() {
    view_idle(0);
}

void view_display_signing_error() {
    view_idle(0);
}
