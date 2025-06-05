#include "MetricsPlugin.h"
#include "../../lib/ethernet/webServer/WebServer.h"
#include "../../lib/wifi/webServer/WebServer.h"

#define METRICS_CONTENT_TYPE "text/plain; version=0.0.4"

#include <ctime>
#ifdef ESP32
#define UPTIME esp_timer_get_time() / 1'000'000.0f 
#else
#define UPTIME millis() / 1000.0f
#endif

#ifdef __HAS_GPS_METRICS
time_t getUnixTimestampUTC(TinyGPSDate& date, TinyGPSTime& time) {
  if (!date.isValid() || !time.isValid())
    return 0;

  std::tm tm;
  tm.tm_year = date.year() - 1900;
  tm.tm_mon= date.month();
  tm.tm_mday = date.day();
  tm.tm_hour= time.hour();
  tm.tm_min = time.minute();
  tm.tm_sec = time.second();

  return mktime(&tm);
}
#endif



void MetricsPlugin::init() {
  VLF("MSG: Plugins, starting: metrics");
  www.on(METRICS_PLUGIN_PATH, HTTP_GET, std::bind(&MetricsPlugin::populateMetrics, this));
}

void MetricsPlugin::populateMetrics() {
    response.clear();
    response += Metric{"memory", "ESP32 Memory usage", "gauge"}
        .entry(Metric::Entry{1.f * ESP.getFreeHeap()}.label("type", "free").label("memory", "heap").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getHeapSize()}.label("type", "size").label("memory", "heap").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getMinFreeHeap()}.label("type", "min-free").label("memory", "heap").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getMaxAllocHeap()}.label("type", "max-alloc").label("memory", "heap").label("unit", "bytes"))
        
        .entry(Metric::Entry{1.f * ESP.getFreePsram()}.label("type", "free").label("memory", "psram").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getPsramSize()}.label("type", "size").label("memory", "psram").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getMinFreePsram()}.label("type", "min-free").label("memory", "psram").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getMaxAllocPsram()}.label("type", "max-alloc").label("memory", "psram").label("unit", "bytes"))
        
        .entry(Metric::Entry{1.f * ESP.getSketchSize()}.label("md5", ESP.getSketchMD5()).label("type", "size").label("memory", "sketch").label("unit", "bytes"))
        .entry(Metric::Entry{1.f * ESP.getFreeSketchSpace()}.label("type", "free").label("memory", "sketch").label("unit", "bytes"))
        .toString();

    response += Metric{"chip", "Chip Info", "gauge"}
        .entry(Metric::Entry{0.f}
            .label("cores", String(ESP.getChipCores()))
            .label("model", String(ESP.getChipModel()))
            .label("revision", String(ESP.getChipRevision()))
            .label("frequency-MHz", String(ESP.getCpuFreqMHz()))
        ).toString();

    Metric wifiMetric = Metric{"wifi", "WiFi Info", "gauge"}
        .entry(Metric::Entry{0.f}
            .label("connected", String(WiFi.isConnected()))
        );
    if(WiFi.isConnected()) {
        response += Metric{"wifi_rssi", "WiFi RSSI", "gauge"}
            .entry(Metric::Entry{static_cast<float>(WiFi.RSSI())}
                .label("ssid", WiFi.SSID())
                .label("channel", String(WiFi.channel()))
            )
            .toString();
        wifiMetric.entries.back()
            .label("ssid", WiFi.SSID())
            .label("channel", String(WiFi.channel()))
            .label("ip", WiFi.localIP().toString())
            .label("gateway", WiFi.gatewayIP().toString())
            .label("subnet", WiFi.subnetMask().toString())
            .label("dns", WiFi.dnsIP().toString())
            .label("hostname", WiFi.getHostname())
            .label("mac", WiFi.macAddress())
            .label("bssid", WiFi.BSSIDstr());
    }

    response += wifiMetric.toString();


    response += Metric{"uptime", "ESP32 uptime", "gauge"}
        .entry(Metric::Entry{UPTIME}.label("unit", "seconds"))
        .toString();
    for(const MetricPopulator &metricPopulator: metricPopulators) {
        response += metricPopulator().toString();
    }
    www.send(200, METRICS_CONTENT_TYPE, response);
}


String MetricsPlugin::Metric::Entry::toString() const {
    uint16_t index = 0;
    String result = "{";
    for(const auto &label: labels) {
        if (index++ > 0) {
            result += ",";
        }
        result += label.first + "=" + label.second;
    }
    result += "} " + String(value, 2);
    return result;
}



String MetricsPlugin::Metric::toString() const {
    String result("# HELP " + name + " " + help + "\n");
    result += "# TYPE " + name + " " + type + "\n";
    for(const auto &entry: entries) {
        result += name + entry.toString() + "\n";
    }
    return result;
}

MetricsPlugin::Metric &MetricsPlugin::Metric::entry(const Metric::Entry &entry) {
    entries.push_back(entry);
    return *this;
}

MetricsPlugin::Metric::Entry &MetricsPlugin::Metric::Entry::label(const String &name, const String &value) {
    labels.push_back({name, value});
    return *this;
}

#ifdef __HAS_GPS_METRICS
void MetricsPlugin::initGpsMetrics(TinyGPSPlus &gps) {
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_fix", "GPS (1: valid fix)", "gauge"}
      .entry(MetricsPlugin::Metric::Entry{gps.location.isValid() && gps.date.isValid() ? 1.f : 0.f}
    );
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_longitude", "GPS Longitude", "gauge"}
      .entry(MetricsPlugin::Metric::Entry{static_cast<float>(gps.location.lng())});
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_latitude", "GPS Latitude", "gauge"}
      .entry(MetricsPlugin::Metric::Entry{static_cast<float>(gps.location.lat())});
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_satellites", "GPS Satellites", "gauge"}
      .entry(MetricsPlugin::Metric::Entry{static_cast<float>(gps.satellites.value())});
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_epoch", "GPS Epoch", "counter"}
      .entry(MetricsPlugin::Metric::Entry{static_cast<float>(getUnixTimestampUTC(gps.date, gps.time))});
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_chars_processed", "GPS chars processed", "counter"}
      .entry(
        MetricsPlugin::Metric::Entry{static_cast<float>(gps.charsProcessed())}
      );
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_sentences_with_fix", "GPS sentences with fix", "counter"}
      .entry(
        MetricsPlugin::Metric::Entry{static_cast<float>(gps.sentencesWithFix())}
      );
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_sentences_passed_checksum", "GPS sentences passed checksum", "counter"}
      .entry(
        MetricsPlugin::Metric::Entry{static_cast<float>(gps.passedChecksum())}
      );
  });
  addMetricPopulator([&gps](){
    return MetricsPlugin::Metric{"gps_sentences_failed_checksum", "GPS sentences failed checksum", "counter"}
      .entry(
        MetricsPlugin::Metric::Entry{static_cast<float>(gps.failedChecksum())}
      );
  });
}
#endif

MetricsPlugin metricsPlugin;
