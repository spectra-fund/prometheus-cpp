#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <thread>

int main() {
  using namespace prometheus;

  // create an http server running on port 8080
  Exposer exposer{"127.0.0.1:8080", 1};

  // create a metrics registry with component=main labels applied to all its
  // metrics
  auto registry = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto& counter_family = BuildCounter()
                             .Name("time_running_seconds_total")
                             .Help("How many seconds is this server running?")
                             .Labels({{"label", "value"}})
                             .Register(*registry);

  auto& summary_family = BuildSummary()
                             .Name("time_running_duration_ms")
                             .Help("How many seconds is this server running?")
                             .Labels({{"label", "value"}})
                             .Register(*registry);

  // add a counter to the metric family
  auto& second_counter = counter_family.Add(
      {{"another_label", "value"}, {"yet_another_label", "value"}});

prometheus::Summary &first_summary =  summary_family.Add({{"label","value"}},
prometheus::Summary::Quantiles{{0.5,0.01,"p50"},{0.90,0.005,"p90"},{0.9,0.001,"p99"}});
  // ask the exposer to scrape the registry on incoming scrapes
  exposer.RegisterCollectable(registry);

  for (;;) {
    auto start = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // increment the counter by one (second)
    second_counter.Increment();
    auto end = std::chrono::system_clock::now();
    first_summary.Observe(std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count());
  }
  return 0;
}
