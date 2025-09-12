#include "oops/format.h"

namespace oops {
// FFloatPoint
template <typename F> FFloatPoint<F> &FFloatPoint<F>::Sci() {
  format_ = SCI;
  return *this;
}

template <typename F> FFloatPoint<F> &FFloatPoint<F>::Fixed() {
  format_ = FIXED;
  return *this;
}

template <typename F>
FFloatPoint<F> &FFloatPoint<F>::SetPrecision(uint8_t precision) {
  precision_ = precision;
  return *this;
}

template <typename F> void FFloatPoint<F>::Output(std::ostream &out) const {
  std::ostringstream oss;
  if (format_ == FIXED) {
    oss << std::fixed;
  } else if (format_ == SCI) {
    oss << std::scientific;
  }
  oss << std::setprecision(precision_);
  oss << f_;
  out << oss.str();
}

template class FFloatPoint<float>;
template class FFloatPoint<double>;

// FTable
FTable &FTable::SetDelim(const std::string &delim) {
  delim_ = delim;
  return *this;
}

FTable &FTable::SetProp(const Prop &prop) {
  table_prop_ = prop;
  col_prop_vec_.clear();
  return *this;
}

FTable &FTable::SetProp(size_t j, const Prop &prop) {
  if (j > col_prop_vec_.size()) {
    col_prop_vec_.resize(j + 1);
  }
  col_prop_vec_[j] = prop;
  return *this;
}

void FTable::Output(std::ostream &out) const {
  std::vector<size_t> col_width_vec;
  for (size_t i{0}; i < table_.size() - 1; ++i) {
    auto &row{table_[i]};
    for (size_t j{0}; j < row.size(); ++j) {
      for (size_t n{col_width_vec.size()}; n < j + 1; ++n) {
        const Prop &prop{GetProp(n)};
        size_t default_width{
            std::max(prop.left_margin + prop.right_margin, prop.min_width)};
        col_width_vec.push_back(default_width);
      }
      const Prop &prop{GetProp(j)};
      col_width_vec[j] =
          std::max(col_width_vec[j],
                   row[j].size() + prop.left_margin + prop.right_margin);
    }
  }
  for (size_t i{0}; i < table_.size() - 1; ++i) {
    auto &row{table_[i]};
    for (size_t j{0}; j < row.size(); ++j) {
      const Prop &prop{GetProp(j)};
      size_t space_num{col_width_vec[j] - row[j].size()};
      size_t left_space_num{0};
      switch (prop.align) {
      case Align::LEFT: {
        left_space_num = prop.left_margin;
        break;
      }
      case Align::CENTER: {
        left_space_num = std::max(space_num / 2, prop.left_margin);
        break;
      }
      case Align::RIGHT: {
        left_space_num = space_num - prop.right_margin;
        break;
      }
      }
      out << std::string(left_space_num, ' ');
      out << row[j];
      out << std::string(space_num - left_space_num, ' ');
      out << delim_;
    }
    out << std::endl;
  }
}

const FTable::Prop &FTable::GetProp(size_t j) const {
  if (j < col_prop_vec_.size()) {
    return col_prop_vec_[j];
  }
  return table_prop_;
}
} // namespace oops
