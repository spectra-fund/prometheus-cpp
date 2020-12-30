#include "prometheus/summary.h"

#include <iomanip>
#include <sstream>

namespace prometheus {

Summary::Summary(const Quantiles& quantiles,
                 const std::chrono::milliseconds max_age, const int age_buckets)
    : quantiles_{quantiles},
      count_{0},
      sum_{0},
      quantile_values_{quantiles_, max_age, age_buckets} {}

void Summary::Observe(const double value) {
  std::lock_guard<std::mutex> lock(mutex_);

  count_ += 1;
  sum_ += value;
  quantile_values_.insert(value);
}

ClientMetric Summary::Collect() const {
  auto metric = ClientMetric{};

  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& quantile : quantiles_) {
    auto metricQuantile = ClientMetric::Quantile{};
    if (quantile.name.empty()) {
      std::stringstream stream;
      if (quantile.quantile <= 0.99D) {
        stream << std::fixed << std::setprecision(2) << quantile.quantile;
        metricQuantile.quantile = stream.str();
      } else if (quantile.quantile <= 0.999D) {
        stream << std::fixed << std::setprecision(3) << quantile.quantile;
      } else {
        stream << std::fixed << quantile.quantile;
      }
    } else {
      metricQuantile.quantile = quantile.name;
    }

    metricQuantile.value = quantile_values_.get(quantile.quantile);
    metric.summary.quantile.push_back(std::move(metricQuantile));
  }
  metric.summary.sample_count = count_;
  metric.summary.sample_sum = sum_;

  return metric;
}

}  // namespace prometheus
