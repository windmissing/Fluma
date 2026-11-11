napi_value configureProgramCache(napi_env env, napi_callback_info info) {
    size_t count = 2;
    napi_value arguments[2];
    size_t length = 0;
    if (napi_get_cb_info(env, info, &count, arguments, nullptr, nullptr) != napi_ok || count < 1 ||
        napi_get_value_string_utf8(env, arguments[0], nullptr, 0, &length) != napi_ok || length > 4095) {
        return text(env, "CACHE_PATH_INVALID");
    }
    std::string path(length + 1, 0);
    if (napi_get_value_string_utf8(env, arguments[0], path.data(), path.size(), &length) != napi_ok) {
        return text(env, "CACHE_PATH_INVALID");
    }
    path.resize(length);
    if (path.rfind("/data/storage/", 0) != 0 || path.find("..") != std::string::npos ||
        path.size() < 6 || path.substr(path.size() - 6) != "/files") {
        return text(env, "CACHE_PATH_INVALID");
    }
    static std::once_flag once;
    try {
        std::call_once(once, [&] {
            if (count == 2) {
                char sha[65] = {};
                size_t copied = 0;
                if (napi_get_value_string_utf8(env, arguments[1], sha, sizeof(sha), &copied) == napi_ok &&
                    copied == 64 && programcache::KeyValid(sha)) {
                    programcache::packageSource = std::string("DECLARED_SHA256:") + sha;
                }
            }
            if (!programcache::Store().Configure(path)) throw std::runtime_error("CACHE_PRELOAD_FAILED");
        });
    } catch (...) {
        return text(env, "CACHE_PRELOAD_FAILED");
    }
    return text(env, "CACHE_PRELOADED");
}
napi_value programCacheState(napi_env env, napi_callback_info) {
    std::ostringstream out;
    out << "{\"configured\":" << (programcache::Store().Ready() ? "true" : "false")
        << ",\"entries\":" << programcache::Store().Count()
        << ",\"receiptWriteFailures\":" << programcache::Store().ReceiptFailures() << "}";
    return text(env, out.str());
}
