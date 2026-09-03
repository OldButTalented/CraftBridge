#include "WebInterface.h"

#include "Config.h"

#ifdef ARDUINO
#include <WiFi.h>
#endif

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace craftbridge {
namespace {

constexpr uint32_t kCanHealthWindowMs = 2000;

const char *qualityText(SignalQuality quality) {
  switch (quality) {
    case SignalQuality::Fresh:
      return "fresh";
    case SignalQuality::Stale:
      return "stale";
    case SignalQuality::Invalid:
      return "invalid";
    case SignalQuality::Missing:
      return "missing";
  }

  return "missing";
}

const char *sessionStateText(SessionState state) {
  switch (state) {
    case SessionState::PassiveStartupWait:
      return "safe_idle_s0_observation";
    case SessionState::ExternalSessionActive:
      return "active";
    case SessionState::StandaloneSessionStarting:
      return "startup_in_progress";
    case SessionState::StandaloneSessionActive:
      return "active";
    case SessionState::StartupBlocked:
      return "startup_blocked";
    case SessionState::StartupFailed:
      return "startup_failed";
    case SessionState::SessionLost:
      return "session_lost";
  }

  return "unknown";
}

const char *sessionSourceText(SessionSource source) {
  switch (source) {
    case SessionSource::External:
      return "external";
    case SessionSource::Standalone:
      return "standalone";
    case SessionSource::None:
      return "none";
  }

  return "none";
}

const char *startupResult(const SessionStatus &status) {
  if (status.state == SessionState::StartupBlocked) {
    return "blocked_invalid_s0";
  }

  if (status.state == SessionState::StartupFailed) {
    return "failed_terminal";
  }

  if (status.state == SessionState::SessionLost) {
    return "session_lost_manual_recovery";
  }

  if (status.source == SessionSource::External) {
    return "external_session_zero_tx";
  }

  if (status.startup_successes > 0) {
    return "passed";
  }

  if (status.startup_attempts > 0) {
    return "in_progress";
  }

  return "not_started";
}

bool showStoredValue(const SignalState &state, uint32_t now_ms) {
  const SignalQuality quality = state.quality(now_ms);
  return quality == SignalQuality::Fresh ||
         quality == SignalQuality::Stale;
}

bool showCurrentValue(const SignalState &state, uint32_t now_ms) {
  return state.quality(now_ms) == SignalQuality::Fresh;
}

void formatUnsignedValue(
    char *output,
    size_t output_size,
    uint32_t value,
    const SignalState &state,
    uint32_t now_ms,
    bool current_only) {
  const bool show = current_only
      ? showCurrentValue(state, now_ms)
      : showStoredValue(state, now_ms);
  if (!show) {
    std::snprintf(output, output_size, "No current data");
    return;
  }

  std::snprintf(
      output,
      output_size,
      "%u",
      static_cast<unsigned>(value));
}

void formatFloatValue(
    char *output,
    size_t output_size,
    float value,
    const char *format,
    const SignalState &state,
    uint32_t now_ms,
    bool current_only) {
  const bool show = current_only
      ? showCurrentValue(state, now_ms)
      : showStoredValue(state, now_ms);
  if (!show) {
    std::snprintf(output, output_size, "No current data");
    return;
  }

  std::snprintf(
      output,
      output_size,
      format,
      static_cast<double>(value));
}

void formatTripTime(
    char *output,
    size_t output_size,
    uint64_t trip_time_ms) {
  const uint64_t total_seconds = trip_time_ms / 1000;
  const uint64_t hours = total_seconds / 3600;
  const uint64_t minutes = (total_seconds / 60) % 60;
  const uint64_t seconds = total_seconds % 60;
  std::snprintf(
      output,
      output_size,
      "%02llu:%02llu:%02llu",
      static_cast<unsigned long long>(hours),
      static_cast<unsigned long long>(minutes),
      static_cast<unsigned long long>(seconds));
}

bool allEngineSignalsFresh(
    const EngineData &engine_data,
    uint32_t now_ms) {
  return engine_data.rpm_state.quality(now_ms) == SignalQuality::Fresh &&
      engine_data.coolant_temperature_state.quality(now_ms) ==
          SignalQuality::Fresh &&
      engine_data.runtime_state.quality(now_ms) == SignalQuality::Fresh &&
      engine_data.oil_pressure_state.quality(now_ms) == SignalQuality::Fresh &&
      engine_data.battery_voltage_state.quality(now_ms) ==
          SignalQuality::Fresh &&
      engine_data.fuel_flow_state.quality(now_ms) == SignalQuality::Fresh;
}

} // namespace

WebInterface::WebInterface()
#ifdef ARDUINO
    : server_(config::kWebServerPort)
#endif
{
}

bool WebInterface::begin() {
#ifdef ARDUINO
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(config::kAccessPointName)) {
    return false;
  }

  // Responses contain only firmware constants and bounded numeric values.
  // Request data is never reflected into HTML or JSON.
  server_.on("/", HTTP_GET, [this]() {
    server_.send(
        200,
        "text/html; charset=utf-8",
        renderDashboardHtml());
  });
  server_.on("/diagnostics", HTTP_GET, [this]() {
    server_.send(
        200,
        "text/html; charset=utf-8",
        renderDiagnosticsHtml());
  });
  server_.on("/api/status", HTTP_GET, [this]() {
    server_.send(
        200,
        "application/json",
        renderStatusJson());
  });
  server_.onNotFound([this]() {
    server_.send(404, "text/plain", "Not found");
  });
  server_.begin();
  return true;
#else
  return false;
#endif
}

void WebInterface::update(
    const EngineData &engine_data,
    const SessionStatus &session_status,
    const CanDiagnostics &can_diagnostics,
    const RuntimeMetrics &runtime_metrics,
    uint32_t now_ms) {
  const uint32_t error_total =
      can_diagnostics.rx_missed +
      can_diagnostics.rx_overrun +
      can_diagnostics.bus_errors;

  if (can_diagnostics.rx_total != last_can_rx_total_) {
    last_can_activity_ms_ = now_ms;
    can_activity_seen_ = true;
  }

  if (error_total != last_can_error_total_) {
    last_can_error_ms_ = now_ms;
    can_error_seen_ = true;
  }

  engine_data_ = engine_data;
  session_status_ = session_status;
  can_diagnostics_ = can_diagnostics;
  fuel_used_liters_ = runtime_metrics.fuelUsedLiters();
  trip_time_ms_ = runtime_metrics.tripTimeMs();
  captured_at_ms_ = now_ms;
  last_can_rx_total_ = can_diagnostics.rx_total;
  last_can_error_total_ = error_total;
  snapshot_ready_ = true;
}

void WebInterface::loop() {
#ifdef ARDUINO
  server_.handleClient();
#endif
}

void WebInterface::clearResponse() {
  response_length_ = 0;
  response_[0] = '\0';
}

void WebInterface::appendText(const char *text) {
  if (response_length_ >= kResponseCapacity - 1) {
    return;
  }

  const size_t remaining = kResponseCapacity - response_length_;
  const size_t text_length = std::strlen(text);
  const size_t copy_length =
      text_length < remaining - 1 ? text_length : remaining - 1;
  std::memcpy(response_ + response_length_, text, copy_length);
  response_length_ += copy_length;
  response_[response_length_] = '\0';
}

void WebInterface::appendFormat(const char *format, ...) {
  if (response_length_ >= kResponseCapacity - 1) {
    return;
  }

  const size_t remaining = kResponseCapacity - response_length_;
  va_list arguments;
  va_start(arguments, format);
  const int written = std::vsnprintf(
      response_ + response_length_,
      remaining,
      format,
      arguments);
  va_end(arguments);

  if (written < 0) {
    response_[response_length_] = '\0';
    return;
  }

  if (static_cast<size_t>(written) >= remaining) {
    response_length_ = kResponseCapacity - 1;
  } else {
    response_length_ += static_cast<size_t>(written);
  }
}

void WebInterface::appendJsonSignal(
    const char *name,
    const char *value,
    const char *unit,
    const SignalState &state,
    bool prepend_comma) {
  const SignalQuality quality = state.quality(captured_at_ms_);
  const uint32_t age_ms = state.seen ? state.ageMs(captured_at_ms_) : 0;

  appendFormat(
      "%s{\"name\":\"%s\",\"value\":\"%s\","
      "\"unit\":\"%s\",\"valid\":%s,\"status\":\"%s\","
      "\"age_ms\":%u}",
      prepend_comma ? "," : "",
      name,
      value,
      unit,
      state.valid ? "true" : "false",
      qualityText(quality),
      static_cast<unsigned>(age_ms));
}

const char *WebInterface::renderDashboardHtml() {
  clearResponse();

  char rpm[32] = "No current data";
  char temperature[32] = "No current data";
  char runtime[32] = "No current data";
  char oil[32] = "No current data";
  char battery[32] = "No current data";
  char fuel_flow[32] = "No current data";
  char fuel_used[32]{};
  char trip_time[32]{};

  if (snapshot_ready_) {
    formatUnsignedValue(
        rpm,
        sizeof(rpm),
        engine_data_.rpm,
        engine_data_.rpm_state,
        captured_at_ms_,
        true);
    formatUnsignedValue(
        temperature,
        sizeof(temperature),
        engine_data_.coolant_temperature_c,
        engine_data_.coolant_temperature_state,
        captured_at_ms_,
        true);
    formatFloatValue(
        runtime,
        sizeof(runtime),
        engine_data_.runtime_hours,
        "%.1f",
        engine_data_.runtime_state,
        captured_at_ms_,
        true);
    formatFloatValue(
        oil,
        sizeof(oil),
        engine_data_.oil_pressure_kpa,
        "%.0f",
        engine_data_.oil_pressure_state,
        captured_at_ms_,
        true);
    formatFloatValue(
        battery,
        sizeof(battery),
        engine_data_.battery_voltage_v,
        "%.2f",
        engine_data_.battery_voltage_state,
        captured_at_ms_,
        true);
    formatFloatValue(
        fuel_flow,
        sizeof(fuel_flow),
        engine_data_.fuel_flow_lph,
        "%.2f",
        engine_data_.fuel_flow_state,
        captured_at_ms_,
        true);
  }

  std::snprintf(
      fuel_used,
      sizeof(fuel_used),
      "%.2f",
      fuel_used_liters_);
  formatTripTime(trip_time, sizeof(trip_time), trip_time_ms_);

  const bool engine_data_available =
      snapshot_ready_ && allEngineSignalsFresh(engine_data_, captured_at_ms_);
  const bool can_ok =
      snapshot_ready_ && can_activity_seen_ &&
      captured_at_ms_ - last_can_activity_ms_ <= kCanHealthWindowMs &&
      (!can_error_seen_ ||
       captured_at_ms_ - last_can_error_ms_ > kCanHealthWindowMs);

  appendText(
      "<!doctype html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>CraftBridge Motor Node</title><style>"
      "*{box-sizing:border-box}body{margin:0;background:#090d12;color:#edf2f7;"
      "font:15px system-ui,sans-serif;overflow-x:hidden}.wrap{max-width:1120px;"
      "margin:auto;padding:18px}header{display:flex;justify-content:space-between;"
      "align-items:end;gap:12px;margin-bottom:14px}h1{font-size:21px;margin:0}"
      "a{color:#83c5ff}.status{display:flex;gap:8px;flex-wrap:wrap;margin:12px 0}"
      ".badge{background:#17202b;border:1px solid #344150;border-radius:16px;"
      "padding:6px 10px}.ok{color:#69e483;border-color:#28783c}.warn{color:#ffbd66;"
      "border-color:#8a5b1d}.grid{display:grid;grid-template-columns:repeat(2,"
      "minmax(0,1fr));gap:10px}.card{min-width:0;min-height:112px;background:#111923;"
      "border:1px solid #283544;border-radius:9px;padding:13px;display:flex;"
      "flex-direction:column;justify-content:space-between}.label{color:#9baaba;"
      "font-size:13px}.reading{font-size:29px;font-weight:650;line-height:1.15;"
      "overflow-wrap:anywhere}.unit{font-size:14px;color:#9baaba;margin-left:4px}"
      ".nodata{font-size:17px;color:#ffbd66}footer{margin-top:14px;color:#8391a0}"
      "@media(min-width:900px){.grid{grid-template-columns:repeat(4,minmax(0,1fr))}}"
      "@media(max-width:359px){.grid{grid-template-columns:1fr}.wrap{padding:10px}}"
      "</style></head><body><div class='wrap'><header><h1>CraftBridge Motor Node"
      "</h1><a href='/diagnostics'>Diagnostics</a></header><div class='status'>");
  appendFormat(
      "<span id='engineStatus' class='badge %s'>%s</span>",
      engine_data_available ? "ok" : "warn",
      engine_data_available
          ? "Engine data available"
          : "Engine data unavailable");
  appendFormat(
      "<span id='canStatus' class='badge %s'>%s</span></div><main class='grid'>",
      can_ok ? "ok" : "warn",
      can_ok ? "CAN OK" : "CAN unavailable");

  const char *labels[] = {
      "Engine speed",
      "Engine temperature",
      "Engine hours",
      "Oil pressure",
      "Battery",
      "Fuel flow",
      "Fuel used",
      "Trip time",
  };
  const char *values[] = {
      rpm,
      temperature,
      runtime,
      oil,
      battery,
      fuel_flow,
      fuel_used,
      trip_time,
  };
  const char *units[] = {"rpm", "°C", "h", "kPa", "V", "L/h", "L", ""};

  for (size_t index = 0; index < 8; ++index) {
    const bool no_data = std::strcmp(values[index], "No current data") == 0;
    appendFormat(
        "<section class='card'><div class='label'>%s</div>"
        "<div><span id='v%u' class='reading%s'>%s</span>"
        "<span id='u%u' class='unit'>%s</span></div></section>",
        labels[index],
        static_cast<unsigned>(index),
        no_data ? " nodata" : "",
        values[index],
        static_cast<unsigned>(index),
        no_data ? "" : units[index]);
  }

  appendText(
      "</main><footer>Live data updates every second.</footer></div><script>"
      "const units=['rpm','°C','h','kPa','V','L/h','L',''];"
      "function card(i,v,ok){const e=document.getElementById('v'+i);"
      "e.textContent=ok?v:'No current data';e.className='reading'+(ok?'':' nodata');"
      "document.getElementById('u'+i).textContent=ok?units[i]:'';}"
      "function badge(id,ok,yes,no){const e=document.getElementById(id);"
      "e.textContent=ok?yes:no;e.className='badge '+(ok?'ok':'warn');}"
      "function update(d){badge('engineStatus',d.engine_data_available,"
      "'Engine data available','Engine data unavailable');"
      "badge('canStatus',d.can_ok,'CAN OK','CAN unavailable');"
      "const p=[0,0,1,0,2,2];for(let i=0;i<6;i++){const s=d.signals[i];"
      "const ok=!!s&&s.status==='fresh';card(i,ok?Number(s.value).toFixed(p[i]):'',ok);}"
      "card(6,Number(d.fuel_used_l).toFixed(2),true);card(7,d.trip_time,true);}"
      "async function poll(){try{const r=await fetch('/api/status',{cache:'no-store'});"
      "if(r.ok)update(await r.json());}catch(e){badge('canStatus',false,'CAN OK',"
      "'CAN unavailable');}}poll();setInterval(poll,1000);</script></body></html>");
  return response_;
}

const char *WebInterface::renderDiagnosticsHtml() {
  clearResponse();
  appendText(
      "<!doctype html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>CraftBridge Diagnostics</title><style>"
      "*{box-sizing:border-box}body{margin:0;background:#090d12;color:#edf2f7;"
      "font:14px system-ui,sans-serif}.wrap{max-width:1000px;margin:auto;padding:18px}"
      "header{display:flex;justify-content:space-between;align-items:end}h1{font-size:21px}"
      "h2{font-size:16px;margin:20px 0 7px}a{color:#83c5ff}table{width:100%;"
      "border-collapse:collapse;background:#111923}th,td{padding:7px;border:1px solid "
      "#2b3948;text-align:left;overflow-wrap:anywhere}.fresh{color:#69e483}.stale,"
      ".invalid{color:#ffbd66}.missing{color:#9baaba}@media(max-width:600px){"
      ".wrap{padding:10px}th,td{padding:5px;font-size:12px}}</style></head><body>"
      "<div class='wrap'><header><h1>Diagnostics</h1><a href='/'>Main</a></header>"
      "<h2>System</h2><table><tbody>"
      "<tr><td>Firmware</td><td id='firmware'>-</td></tr>"
      "<tr><td>Uptime</td><td id='uptime'>-</td></tr>"
      "<tr><td>Session state / source</td><td id='session'>-</td></tr>"
      "<tr><td>Startup result</td><td id='startup'>-</td></tr>"
      "<tr><td>S3 detected</td><td id='s3'>-</td></tr>"
      "<tr><td>Application TX</td><td id='tx'>-</td></tr></tbody></table>"
      "<h2>CAN</h2><table><tbody>"
      "<tr><td>RX total / standard / extended</td><td id='rx'>-</td></tr>"
      "<tr><td>RTR / malformed</td><td id='other'>-</td></tr>"
      "<tr><td>Missed / overrun / bus</td><td id='errors'>-</td></tr></tbody></table>"
      "<h2>Signals</h2><table><thead><tr><th>Signal</th><th>Value</th><th>Validity</th>"
      "<th>Freshness</th><th>Age ms</th></tr></thead><tbody id='signals'></tbody></table>"
      "</div><script>function text(id,v){document.getElementById(id).textContent=v;}"
      "function update(d){text('firmware',d.firmware);text('uptime',d.uptime_ms+' ms');"
      "text('session',d.session+' / '+d.session_source);text('startup',d.startup_result);"
      "text('s3',d.s3_detected?'yes':'no');text('tx',(d.application_tx_enabled?"
      "'enabled':'0 / disabled')+'; count '+d.application_tx_count);"
      "text('rx',d.can_rx_total+' / '+d.can_rx_standard+' / '+d.can_rx_extended);"
      "text('other',d.can_rx_rtr+' / '+d.can_rx_malformed);text('errors',"
      "d.can_rx_missed+' / '+d.can_rx_overrun+' / '+d.can_bus_errors);"
      "const b=document.getElementById('signals');b.textContent='';for(const s of d.signals){"
      "const r=document.createElement('tr');const current=s.status==='fresh';"
      "for(const v of [s.name,current?s.value+' '+s.unit:'No current data',"
      "s.valid?'yes':'no',s.status,s.age_ms]){const c=document.createElement('td');"
      "c.textContent=v;r.appendChild(c);}r.children[3].className=s.status;b.appendChild(r);}}"
      "async function poll(){try{const r=await fetch('/api/status',{cache:'no-store'});"
      "if(r.ok)update(await r.json());}catch(e){}}poll();setInterval(poll,1000);"
      "</script></body></html>");
  return response_;
}

const char *WebInterface::renderStatusJson() {
  clearResponse();

  const char *firmware = snapshot_ready_ ? config::kFirmwareIdentity : "";
  const char *session_state =
      snapshot_ready_ ? sessionStateText(session_status_.state) : "";
  const char *session_source =
      snapshot_ready_ ? sessionSourceText(session_status_.source) : "";
  const char *startup_result =
      snapshot_ready_ ? startupResult(session_status_) : "";
  const bool engine_data_available =
      snapshot_ready_ && allEngineSignalsFresh(engine_data_, captured_at_ms_);
  const bool can_ok =
      snapshot_ready_ && can_activity_seen_ &&
      captured_at_ms_ - last_can_activity_ms_ <= kCanHealthWindowMs &&
      (!can_error_seen_ ||
       captured_at_ms_ - last_can_error_ms_ > kCanHealthWindowMs);
  char trip_time[32]{};
  formatTripTime(trip_time, sizeof(trip_time), trip_time_ms_);

  appendFormat(
      "{\"firmware\":\"%s\",\"uptime_ms\":%u,"
      "\"session\":\"%s\",\"session_source\":\"%s\","
      "\"startup_result\":\"%s\",\"s3_detected\":%s,"
      "\"application_tx_enabled\":%s,\"application_tx_count\":%u,"
      "\"can_rx_total\":%u,\"can_rx_standard\":%u,"
      "\"can_rx_extended\":%u,\"can_rx_rtr\":%u,"
      "\"can_rx_malformed\":%u,\"can_rx_missed\":%u,"
      "\"can_rx_overrun\":%u,\"can_bus_errors\":%u,"
      "\"can_ok\":%s,\"engine_data_available\":%s,"
      "\"fuel_used_l\":%.6f,\"trip_time_ms\":%llu,"
      "\"trip_time\":\"%s\",\"signals\":[",
      firmware,
      static_cast<unsigned>(captured_at_ms_),
      session_state,
      session_source,
      startup_result,
      session_status_.s3_complete ? "true" : "false",
      session_status_.application_tx_enabled ? "true" : "false",
      static_cast<unsigned>(can_diagnostics_.tx_successful),
      static_cast<unsigned>(can_diagnostics_.rx_total),
      static_cast<unsigned>(can_diagnostics_.rx_standard),
      static_cast<unsigned>(can_diagnostics_.rx_extended),
      static_cast<unsigned>(can_diagnostics_.rx_rtr),
      static_cast<unsigned>(can_diagnostics_.rx_malformed),
      static_cast<unsigned>(can_diagnostics_.rx_missed),
      static_cast<unsigned>(can_diagnostics_.rx_overrun),
      static_cast<unsigned>(can_diagnostics_.bus_errors),
      can_ok ? "true" : "false",
      engine_data_available ? "true" : "false",
      fuel_used_liters_,
      static_cast<unsigned long long>(trip_time_ms_),
      trip_time);

  if (!snapshot_ready_) {
    const SignalState missing_state{};
    for (size_t index = 0; index < 6; ++index) {
      appendJsonSignal(
          "",
          "No current data",
          "",
          missing_state,
          index > 0);
    }
  } else {
    char rpm[32]{};
    char temperature[32]{};
    char runtime[32]{};
    char oil[32]{};
    char battery[32]{};
    char fuel_flow[32]{};

    formatUnsignedValue(
        rpm,
        sizeof(rpm),
        engine_data_.rpm,
        engine_data_.rpm_state,
        captured_at_ms_,
        false);
    formatUnsignedValue(
        temperature,
        sizeof(temperature),
        engine_data_.coolant_temperature_c,
        engine_data_.coolant_temperature_state,
        captured_at_ms_,
        false);
    formatFloatValue(
        runtime,
        sizeof(runtime),
        engine_data_.runtime_hours,
        "%.1f",
        engine_data_.runtime_state,
        captured_at_ms_,
        false);
    formatFloatValue(
        oil,
        sizeof(oil),
        engine_data_.oil_pressure_kpa,
        "%.2f",
        engine_data_.oil_pressure_state,
        captured_at_ms_,
        false);
    formatFloatValue(
        battery,
        sizeof(battery),
        engine_data_.battery_voltage_v,
        "%.2f",
        engine_data_.battery_voltage_state,
        captured_at_ms_,
        false);
    formatFloatValue(
        fuel_flow,
        sizeof(fuel_flow),
        engine_data_.fuel_flow_lph,
        "%.2f",
        engine_data_.fuel_flow_state,
        captured_at_ms_,
        false);

    appendJsonSignal(
        "Engine RPM",
        rpm,
        "rpm",
        engine_data_.rpm_state,
        false);
    appendJsonSignal(
        "Engine temperature",
        temperature,
        "°C",
        engine_data_.coolant_temperature_state,
        true);
    appendJsonSignal(
        "Engine hours",
        runtime,
        "h",
        engine_data_.runtime_state,
        true);
    appendJsonSignal(
        "Oil pressure",
        oil,
        "kPa",
        engine_data_.oil_pressure_state,
        true);
    appendJsonSignal(
        "Battery voltage",
        battery,
        "V",
        engine_data_.battery_voltage_state,
        true);
    appendJsonSignal(
        "Fuel flow",
        fuel_flow,
        "l/h",
        engine_data_.fuel_flow_state,
        true);
  }

  appendText("]}");
  return response_;
}

} // namespace craftbridge
