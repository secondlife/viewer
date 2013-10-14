
// required symbols to grab
LL_DBUS_SYM(true, dbus_g_bus_get, DBusGConnection*, DBusBusType, GError**);
LL_DBUS_SYM(true, dbus_g_proxy_new_for_name, DBusGProxy*, DBusGConnection*, const char *, const char*, const char*);
LL_DBUS_SYM(true, dbus_g_proxy_call, gboolean, DBusGProxy*, const char*, GError**, GType, ...);
LL_DBUS_SYM(true, dbus_g_object_type_install_info, void, GType, const DBusGObjectInfo*);
LL_DBUS_SYM(true, dbus_g_connection_register_g_object, void, DBusGConnection*, const char*, GObject*);

// optional symbols to grab
