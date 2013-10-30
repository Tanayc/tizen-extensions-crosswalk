// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include "common/picojson.h"

const std::string SysInfoStorage::name_ = "STORAGE";

void SysInfoStorage::Get(picojson::value& error,
                         picojson::value& data) {
  if (!Update(error)) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get storage faild."));
    return;
  }

  system_info::SetPicoJsonObjectValue(data, "units", units_);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

gboolean SysInfoStorage::OnUpdateTimeout(gpointer user_data) {
  SysInfoStorage* instance = static_cast<SysInfoStorage*>(user_data);

  // Can't to take a reference (&), just copy.
  picojson::array old_units_arr = instance->units_.get<picojson::array>();
  picojson::value error = picojson::value(picojson::object());
  instance->Update(error);

  bool is_changed = false;
  picojson::array& units_arr = instance->units_.get<picojson::array>();
  if (old_units_arr.size() != units_arr.size()) {
    is_changed = true;
  } else {
    for (unsigned int i = 0; i < units_arr.size(); ++i) {
      if (old_units_arr[i] != units_arr[i]) {
        is_changed = true;
        break;
      }
    }
  }

  if (is_changed) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "units", instance->units_);
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("STORAGE"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    instance->PostMessageToListeners(output);
  }

  return TRUE;
}

void SysInfoStorage::StartListening() {
  // FIXME(halton): Use udev D-Bus interface to monitor.
  if (timeout_cb_id_ == 0) {
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoStorage::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
}

void SysInfoStorage::StopListening() {
  if (timeout_cb_id_ > 0) {
    g_source_remove(timeout_cb_id_);
    timeout_cb_id_ = 0;
  }
}
