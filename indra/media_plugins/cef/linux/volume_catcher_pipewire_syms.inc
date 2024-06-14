#define G pwSymbolGrabber

// required symbols to grab
LL_GRAB_SYM(G, true, pw_init, void, int *argc, char **argv[]);
// LL_GRAB_SYM(G, true, pw_main_loop_new, struct pw_main_loop *, const struct spa_dict *props);
// LL_GRAB_SYM(G, true, pw_main_loop_get_loop, struct pw_loop *, struct pw_main_loop *loop);
// LL_GRAB_SYM(G, true, pw_main_loop_destroy, void, struct pw_main_loop *loop);
// LL_GRAB_SYM(G, true, pw_main_loop_run, void, struct pw_main_loop *loop);
LL_GRAB_SYM(G, true, pw_context_new, struct pw_context *, struct pw_loop *main_loop, struct pw_properties *props, size_t user_data_size);
LL_GRAB_SYM(G, true, pw_context_destroy, void, struct pw_context *context);
LL_GRAB_SYM(G, true, pw_context_connect, struct pw_core *, struct pw_context *context, struct pw_properties *properties, size_t user_data_size);
LL_GRAB_SYM(G, true, pw_thread_loop_new, struct pw_thread_loop *, const char *name, const struct spa_dict *props);
LL_GRAB_SYM(G, true, pw_thread_loop_destroy, void, struct pw_thread_loop *loop);
LL_GRAB_SYM(G, true, pw_thread_loop_get_loop, struct pw_loop *, struct pw_thread_loop *loop);
LL_GRAB_SYM(G, true, pw_thread_loop_start, int, struct pw_thread_loop *loop);
LL_GRAB_SYM(G, true, pw_thread_loop_stop, void, struct pw_thread_loop *loop);
LL_GRAB_SYM(G, true, pw_thread_loop_lock, void, struct pw_thread_loop *loop);
LL_GRAB_SYM(G, true, pw_thread_loop_unlock, void, struct pw_thread_loop *loop);
LL_GRAB_SYM(G, true, pw_proxy_add_listener, void, struct pw_proxy *proxy, struct spa_hook *listener, const struct pw_proxy_events *events, void *data);
LL_GRAB_SYM(G, true, pw_proxy_destroy, void, struct pw_proxy *proxy);
LL_GRAB_SYM(G, true, pw_proxy_get_user_data, void *, struct pw_proxy *proxy);
LL_GRAB_SYM(G, true, pw_core_disconnect, int, struct pw_core *core);

// optional symbols to grab

#undef G
