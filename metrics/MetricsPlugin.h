// Metrics plugin
#pragma once
#include <Arduino.h>
#include <list>
#include "../../Common.h"

#define HAS_METRICS_PLUGIN

#if defined(TIME_LOCATION_SOURCE) && TIME_LOCATION_SOURCE == GPS
#define __HAS_GPS_METRICS
#include <TinyGPS++.h>
#endif

#ifndef METRICS_PLUGIN_PATH
#define METRICS_PLUGIN_PATH "/metrics"
#endif

class MetricsPlugin{
public:
  void init();
  void populateMetrics();

#ifdef __HAS_GPS_METRICS
  void initGpsMetrics(TinyGPSPlus &gps);
#endif


struct Metric {
    String name;
    String help;
    String type;
    struct Entry {
        using Label = std::pair<String, String>;
        float value;
        std::list<Label> labels;
        String toString() const;
        Entry &label(const String &name, const String &value);
    };
    std::list<Entry> entries;
    String toString() const;
    Metric &entry(const Metric::Entry &entry);
};

using MetricPopulator = std::function<Metric()>;
void addMetricPopulator(const MetricPopulator &metricPopulator) {
    metricPopulators.push_back(metricPopulator);
}

private:
    String response;
    std::list<MetricPopulator> metricPopulators;
};

extern MetricsPlugin metricsPlugin;
