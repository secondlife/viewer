// required symbols to grab
LL_PA_SYM(true, pa_context_connect, int, pa_context *c, const char *server, pa_context_flags_t flags, const pa_spawn_api *api);
LL_PA_SYM(true, pa_context_disconnect, void, pa_context *c);
LL_PA_SYM(true, pa_context_get_sink_input_info, pa_operation*, pa_context *c, uint32_t idx, pa_sink_input_info_cb_t cb, void *userdata);
LL_PA_SYM(true, pa_context_get_sink_input_info_list, pa_operation*, pa_context *c, pa_sink_input_info_cb_t cb, void *userdata);
LL_PA_SYM(true, pa_context_get_state, pa_context_state_t, pa_context *c);
LL_PA_SYM(true, pa_context_new_with_proplist, pa_context*, pa_mainloop_api *mainloop, const char *name, pa_proplist *proplist);
LL_PA_SYM(true, pa_context_set_sink_input_volume, pa_operation*, pa_context *c, uint32_t idx, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);
LL_PA_SYM(true, pa_context_set_state_callback, void, pa_context *c, pa_context_notify_cb_t cb, void *userdata);
LL_PA_SYM(true, pa_context_set_subscribe_callback, void, pa_context *c, pa_context_subscribe_cb_t cb, void *userdata);
LL_PA_SYM(true, pa_context_subscribe, pa_operation*, pa_context *c, pa_subscription_mask_t m, pa_context_success_cb_t cb, void *userdata);
LL_PA_SYM(true, pa_context_unref, void, pa_context *c);
LL_PA_SYM(true, pa_cvolume_set, pa_cvolume*, pa_cvolume *a, unsigned channels, pa_volume_t v);
LL_PA_SYM(true, pa_operation_unref, void, pa_operation *o);
LL_PA_SYM(true, pa_proplist_free, void, pa_proplist* p);
LL_PA_SYM(true, pa_proplist_gets, const char*, pa_proplist *p, const char *key);
LL_PA_SYM(true, pa_proplist_new, pa_proplist*, void);
LL_PA_SYM(true, pa_proplist_sets, int, pa_proplist *p, const char *key, const char *value);
LL_PA_SYM(true, pa_sw_volume_from_linear, pa_volume_t, double v);

// optional symbols to grab
