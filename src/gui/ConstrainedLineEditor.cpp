#include "gui/ConstrainedLineEditor.h"
#include "gui/SketchEditor.h"
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <limits>
namespace designrc::gui {
namespace {
QPainterPath anchorPath(const SketchEditor& source, int layer, int curve) {
  if (layer < 0 || layer >= static_cast<int>(source.layers().size())) return {};
  const auto& outline = source.layers()[layer];
  if (curve < 0 || curve >= static_cast<int>(outline.curves.size())) return {};
  const auto& segment = outline.curves[curve];
  std::vector<QPointF> points;
  for (auto id : segment.points) points.push_back(outline.points[id]);
  return SketchEditor::fittedPath(points, segment.type);
}
double distance(QPointF a, QPointF b) { return QLineF{a, b}.length(); }
}
ConstrainedLineEditor::ConstrainedLineEditor(QGraphicsView& view, SketchEditor& source)
    : view_{view}, source_{source} { view_.viewport()->setMouseTracking(true); }
void ConstrainedLineEditor::notify() { view_.viewport()->update(); emit source_.stationsChanged(); }
void ConstrainedLineEditor::cancel() {
  if (moving_ >= 0 && beforeMove_) lines_[moving_] = *beforeMove_;
  verticalPreview_.reset();
  first_.reset(); hover_.reset(); beforeMove_.reset(); moving_ = -1; selected_ = -1;
  view_.viewport()->update();
}
void ConstrainedLineEditor::dropMove() {
  // lines_ already contains the latest valid constrained position. Clear the
  // move before cancel() so cleanup cannot restore the original position.
  moving_ = -1;
  cancel();
  notify();
}
void ConstrainedLineEditor::setEnabled(bool enabled) {
  if (enabled) selectionOnly_ = false;
  if (enabled_ == enabled) return;
  cancel(); enabled_ = enabled;
  view_.viewport()->setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
  if (enabled) view_.setFocus();
}
void ConstrainedLineEditor::setSelectionEnabled(bool enabled) {
  if (selectionOnly_ == enabled) return;
  const int previous = selected_;
  if (enabled) setEnabled(false);
  cancel(); selectionOnly_ = enabled;
  selected_ = previous >= 0 && previous < static_cast<int>(lines_.size()) && lines_[previous].first.layer==panel_ ? previous : -1;
  if(enabled && selected_<0)for(int i=0;i<static_cast<int>(lines_.size());++i)if(lines_[i].first.layer==panel_){selected_=i;break;}
  emit source_.stationSelectionChanged(selected_);
}
void ConstrainedLineEditor::setActivePanel(int panel) {
  panel=std::clamp(panel,0,static_cast<int>(source_.layers().size())-1);
  if(panel_==panel)return;
  if(moving_>=0)dropMove();else cancel();
  panel_=panel;
  if(selectionOnly_) {
    for(int i=0;i<static_cast<int>(lines_.size());++i)if(lines_[i].first.layer==panel_){selected_=i;break;}
    emit source_.stationSelectionChanged(selected_);
  }
  view_.viewport()->update();
}
bool ConstrainedLineEditor::allPanelsDefined() const {
  for(const auto& line:lines_)if(line.first.layer!=line.second.layer)return false;
  for(int panel=0;panel<static_cast<int>(source_.layers().size());++panel)
    if(std::count_if(lines_.begin(),lines_.end(),[&](const auto& line){return line.first.layer==panel && line.second.layer==panel;})<2)return false;
  return true;
}
void ConstrainedLineEditor::assignSelectedAirfoil(std::size_t airfoil) {
  if (selected_ < 0 || selected_ >= static_cast<int>(lines_.size())) return;
  if (lines_[selected_].airfoil == airfoil) return;
  lines_[selected_].airfoil = airfoil; notify();
}
void ConstrainedLineEditor::assignSelectedProfile(std::optional<std::size_t> profile) {
  if(selected_<0 || selected_>=static_cast<int>(lines_.size()))return;
  if(lines_[selected_].profile==profile)return;
  lines_[selected_].profile=profile;notify();
}
void ConstrainedLineEditor::setThicknessMm(int station,double thickness,std::optional<LengthUnit> unit) {
  if(station<0 || station>=static_cast<int>(lines_.size()) || !std::isfinite(thickness) || thickness<=0)return;
  if(lines_[station].thicknessMm==thickness&&(!unit||lines_[station].thicknessUnit==*unit))return;
  if(unit)lines_[station].thicknessUnit=*unit;
  lines_[station].thicknessMm=thickness;notify();
}
void ConstrainedLineEditor::reset() {
  cancel(); panel_=0;lines_.clear(); enabled_ = false; selectionOnly_ = false; notify();
}
void ConstrainedLineEditor::restoreState(const StationState& state) {
  cancel(); lines_=state.lines; selected_=state.selected; first_=state.first;
  if(selected_>=0 && lines_[selected_].first.layer!=panel_)selected_=-1;
  if(selectionOnly_ && selected_<0)for(int i=0;i<static_cast<int>(lines_.size());++i)if(lines_[i].first.layer==panel_){selected_=i;break;}
  view_.viewport()->update();
}
std::optional<CurveAnchor> ConstrainedLineEditor::project(QPointF point, int layer, int curve, bool requireNear) const {
  std::optional<CurveAnchor> result;
  double best = requireNear ? 8.0 / view_.transform().m11() : std::numeric_limits<double>::max();
  for (int l = 0; l < static_cast<int>(source_.layers().size()); ++l) {
    if ((layer >= 0 && layer != l) || (layer<0 && l!=panel_)) continue;
    for (int c = 0; c < static_cast<int>(source_.layers()[l].curves.size()); ++c) {
      if (curve >= 0 && curve != c) continue;
      const auto path = anchorPath(source_, l, c);
      const double length = path.length();
      if (length < 1e-9) continue;
      double travelled = 0;
      for (int i = 1; i < path.elementCount(); ++i) {
        const QPointF a{path.elementAt(i - 1).x, path.elementAt(i - 1).y};
        const QPointF b{path.elementAt(i).x, path.elementAt(i).y};
        const QPointF delta = b - a;
        const double segmentLength = distance(a, b);
        if (segmentLength < 1e-12) continue;
        const double t = std::clamp(QPointF::dotProduct(point - a, delta) / (segmentLength * segmentLength), 0.0, 1.0);
        const QPointF candidate = a + t * delta;
        const double d = distance(point, candidate);
        if (d <= best) {
          best = d; result = CurveAnchor{l, c, (travelled + t * segmentLength) / length, candidate};
        }
        travelled += segmentLength;
      }
    }
  }
  return result;
}
std::optional<CurveAnchor> ConstrainedLineEditor::intersect(const CurveAnchor& onCurve, QPointF through, LineAlignment alignment) const {
  const auto path = anchorPath(source_, onCurve.layer, onCurve.curve);
  const double length = path.length();
  if (length < 1e-9) return {};
  const auto coordinate = [alignment](QPointF p) { return alignment == LineAlignment::Vertical ? p.x() : p.y(); };
  std::optional<CurveAnchor> result;
  double best = std::numeric_limits<double>::max(), travelled = 0;
  for (int i = 1; i < path.elementCount(); ++i) {
    const QPointF a{path.elementAt(i - 1).x, path.elementAt(i - 1).y};
    const QPointF b{path.elementAt(i).x, path.elementAt(i).y};
    const double segmentLength = distance(a, b), delta = coordinate(b) - coordinate(a);
    if (std::abs(delta) > 1e-12) {
      const double t = (coordinate(through) - coordinate(a)) / delta;
      if (t >= -1e-9 && t <= 1 + 1e-9) {
        const auto point = a + std::clamp(t, 0.0, 1.0) * (b - a);
        const auto d = distance(point, onCurve.position);
        if (d < best) {
          best = d; result = CurveAnchor{onCurve.layer, onCurve.curve,
              (travelled + std::clamp(t, 0.0, 1.0) * segmentLength) / length, point};
        }
      }
    } else if (std::abs(coordinate(through) - coordinate(a)) < 1e-8) {
      // Coincident axis/edge: retain the nearest position on this source curve.
      result = onCurve;
    }
    travelled += segmentLength;
  }
  return result;
}
std::optional<ConstrainedLine> ConstrainedLineEditor::verticalSection(double x,int layer) const {
  if(layer<0 || layer>=static_cast<int>(source_.layers().size()))return {};
  std::vector<CurveAnchor> hits;
  for(int c=0;c<static_cast<int>(source_.layers()[layer].curves.size());++c) {
    const auto path=anchorPath(source_,layer,c);
    const double length=path.length();if(length<1e-9)continue;
    double travelled=0;
    auto add=[&](QPointF point,double arc) {
      point.setX(x);
      if(std::none_of(hits.begin(),hits.end(),[&](const auto& hit){return distance(hit.position,point)<1e-7;}))
        hits.push_back({layer,c,arc/length,point});
    };
    for(int i=1;i<path.elementCount();++i) {
      const QPointF a{path.elementAt(i-1).x,path.elementAt(i-1).y},b{path.elementAt(i).x,path.elementAt(i).y};
      const double segment=distance(a,b),dx=b.x()-a.x();
      if(std::abs(dx)>1e-12) {
        const double t=(x-a.x())/dx;
        if(t>=-1e-9&&t<=1+1e-9)add(a+std::clamp(t,0.,1.)*(b-a),travelled+std::clamp(t,0.,1.)*segment);
      } else if(std::abs(x-a.x())<1e-8) {add(a,travelled);add(b,travelled+segment);}
      travelled+=segment;
    }
  }
  // More than two intersections is ambiguous (e.g. a folded or concave outline).
  // Do not silently connect across exterior space. A pointed end has zero height.
  if(hits.size()!=2)return {};
  if(hits[0].position.y()>hits[1].position.y())std::swap(hits[0],hits[1]);
  if(hits[1].position.y()-hits[0].position.y()<1e-7)return {};
  return ConstrainedLine{hits[0],hits[1],LineAlignment::Vertical};
}
bool ConstrainedLineEditor::duplicateVertical(const ConstrainedLine& line,int except) const {
  for(int i=0;i<static_cast<int>(lines_.size());++i)
    if(i!=except&&lines_[i].first.layer==line.first.layer &&
        std::abs(lines_[i].first.position.x()-line.first.position.x())<1e-6)return true;
  return false;
}
void ConstrainedLineEditor::updateHover(QPointF point) {
  if(verticalPlacement_) {
    verticalPreview_.reset();
    if(moving_>=0) {
      const auto& anchor=movingEnd_==0?beforeMove_->first:beforeMove_->second;
      hover_=project(point,anchor.layer,anchor.curve,false);
    } else hover_=project(point);
    if(hover_) {
      auto section=verticalSection(hover_->position.x(),hover_->layer);
      if(section && std::min(distance(hover_->position,section->first.position),distance(hover_->position,section->second.position))<1e-6) {
        verticalPreview_=section;
        if(moving_>=0 && !duplicateVertical(*section,moving_)) {
          section->thicknessMm=lines_[moving_].thicknessMm;section->profile=lines_[moving_].profile;lines_[moving_]=*section;
        }
      } else hover_.reset();
    }
    view_.viewport()->update();return;
  }

  if (moving_ >= 0) {
    auto candidate = *beforeMove_;
    auto& moving = movingEnd_ == 0 ? candidate.first : candidate.second;
    auto& other = movingEnd_ == 0 ? candidate.second : candidate.first;
    hover_ = project(point, moving.layer, moving.curve, false);
    if (hover_) {
      moving = *hover_;
      if (candidate.alignment != LineAlignment::Free) {
        const auto paired = intersect(other, moving.position, candidate.alignment);
        if (!paired) { hover_.reset(); view_.viewport()->update(); return; }
        other = *paired;
      }
      if (distance(moving.position, other.position) > 1e-8) lines_[moving_] = candidate;
    }
  } else {
    hover_ = project(point);
    previewAlignment_ = LineAlignment::Free;
    if (first_ && hover_) {
      const auto delta = hover_->position - first_->position;
      constexpr double tangent15 = 0.2679491924311227;
      if (distance(hover_->position, first_->position) < 1e-8) { hover_.reset(); view_.viewport()->update(); return; }
      if (std::abs(delta.x()) <= std::abs(delta.y()) * tangent15) previewAlignment_ = LineAlignment::Vertical;
      else if (std::abs(delta.y()) <= std::abs(delta.x()) * tangent15) previewAlignment_ = LineAlignment::Horizontal;
      if (previewAlignment_ != LineAlignment::Free) {
        const auto snapped = intersect(*hover_, first_->position, previewAlignment_);
        if (snapped) hover_ = snapped;
        else previewAlignment_ = LineAlignment::Free;
      }
    }
  }
  view_.viewport()->update();
}
bool ConstrainedLineEditor::event(QEvent* event, bool onViewport) {
  if (selectionOnly_) {
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
      return true;
    }
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Delete) return true;
    if (!onViewport || event->type() != QEvent::MouseButtonPress) return false;
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() != Qt::LeftButton) return false;
    view_.setFocus();
    const auto point = view_.mapToScene(mouse->position().toPoint());
    for (int i = static_cast<int>(lines_.size()) - 1; i >= 0; --i) {
    if(lines_[i].first.layer!=panel_)continue;
      const auto a = lines_[i].first.position, delta = lines_[i].second.position - a;
      const double squared = QPointF::dotProduct(delta, delta);
      if (squared < 1e-16) continue;
      const double t = std::clamp(QPointF::dotProduct(point - a, delta) / squared, 0.0, 1.0);
      if (distance(point, a + t * delta) <= 8 / view_.transform().m11()) { selected_ = i; break; }
    }
    view_.viewport()->update(); emit source_.stationSelectionChanged(selected_); return true;
  }
  if (!enabled_) return false;
  if (event->type() == QEvent::KeyPress) {
    const int key = static_cast<QKeyEvent*>(event)->key();
    if (key == Qt::Key_Escape) {
      if (moving_ >= 0) dropMove();
      else cancel();
      return true;
    }
    if (key == Qt::Key_Delete) {
      if (selected_ >= 0 && moving_ < 0) { lines_.erase(lines_.begin() + selected_); cancel(); notify(); }
      return true;
    }
  }
  if (!onViewport) return false;
  if (event->type() == QEvent::Leave) { verticalPreview_.reset(); hover_.reset(); view_.viewport()->update(); }
  if (event->type() == QEvent::MouseMove) {
    updateHover(view_.mapToScene(static_cast<QMouseEvent*>(event)->position().toPoint())); return true;
  }
  if (event->type() != QEvent::MouseButtonPress) return false;
  auto* mouse = static_cast<QMouseEvent*>(event);
  if (mouse->button() != Qt::LeftButton) return false;
  view_.setFocus();
  const auto point = view_.mapToScene(mouse->position().toPoint());
  updateHover(point);
  if (moving_ >= 0) {
    dropMove();
    return true;
  }
  if (first_) {
    if (hover_ && distance(first_->position, hover_->position) > 1e-8) {
      const bool duplicate = std::any_of(lines_.begin(), lines_.end(), [&](const auto& line) {
        return line.first.layer==panel_ && ((distance(line.first.position, first_->position) < 1e-6 && distance(line.second.position, hover_->position) < 1e-6) ||
            (distance(line.second.position, first_->position) < 1e-6 && distance(line.first.position, hover_->position) < 1e-6));
      });
      const ConstrainedLine candidate{*first_,*hover_,previewAlignment_};
      const auto conflict=std::find_if(lines_.begin(),lines_.end(),[&](const auto& line){return !stationDirectionsAgree(candidate,line);});
      if(!duplicate && conflict!=lines_.end()) {
        const int otherPanel=conflict->first.layer;
        first_.reset();hover_.reset();notify();
        QMessageBox::warning(&view_,"Conflicting LE/TE orientation",
          QString{"This station on panel %1 reverses the leading/trailing edge direction of an existing station on panel %2. The station was not added. Click the leading edge first, then the trailing edge. If the existing station is reversed, delete it and redraw it."}.arg(panel_+1).arg(otherPanel+1));
        return true;
      }
      if (!duplicate) lines_.push_back(candidate);
      first_.reset(); hover_.reset(); notify();
    }
    return true;
  }
  const double tolerance = 8.0 / view_.transform().m11();
  for (int i = static_cast<int>(lines_.size()) - 1; i >= 0; --i) {
    if(lines_[i].first.layer!=panel_)continue;
    for (int end = 0; end < 2; ++end) {
      const auto& anchor = end == 0 ? lines_[i].first : lines_[i].second;
      if (distance(point, anchor.position) <= tolerance) {
        moving_ = i; movingEnd_ = end; beforeMove_ = lines_[i]; selected_ = -1;
        hover_ = anchor; view_.viewport()->update(); return true;
      }
    }
  }
  selected_ = -1;
  for (int i = static_cast<int>(lines_.size()) - 1; i >= 0; --i) {
    if(lines_[i].first.layer!=panel_)continue;
    const auto a = lines_[i].first.position, b = lines_[i].second.position, delta = b - a;
    const double squared = QPointF::dotProduct(delta, delta);
    if (squared < 1e-16) continue;
    const double t = std::clamp(QPointF::dotProduct(point - a, delta) / squared, 0.0, 1.0);
    if (distance(point, a + t * delta) <= tolerance) { selected_ = i; hover_.reset(); break; }
  }
  if (selected_ < 0 && hover_) {
    if(verticalPlacement_) {
      if(verticalPreview_&&!duplicateVertical(*verticalPreview_)) {
        lines_.push_back(*verticalPreview_);notify();
      }
      verticalPreview_.reset();hover_.reset();
    } else first_ = hover_;
  }
  view_.viewport()->update(); return true;
}
void ConstrainedLineEditor::sourceCurveRemoved(int layer, int curve) {
  cancel();
  std::erase_if(lines_, [=](const auto& line) {
    return (line.first.layer == layer && line.first.curve == curve) ||
        (line.second.layer == layer && line.second.curve == curve);
  });
  for (auto& line : lines_) for (auto* anchor : {&line.first, &line.second})
    if (anchor->layer == layer && anchor->curve > curve) --anchor->curve;
}
void ConstrainedLineEditor::synchronize() {
  std::erase_if(lines_, [&](auto& line) {
    if(verticalPlacement_) {
      const auto path=anchorPath(source_,line.first.layer,line.first.curve);
      if(path.isEmpty())return true;
      const auto point=path.pointAtPercent(line.first.parameter);
      auto section=verticalSection(point.x(),line.first.layer);
      if(!section)return true;
      section->thicknessMm=line.thicknessMm;section->profile=line.profile;line=*section;return false;
    }
    for (auto* anchor : {&line.first, &line.second}) {
      const auto path = anchorPath(source_, anchor->layer, anchor->curve);
      if (path.isEmpty()) return true;
      anchor->position = path.pointAtPercent(anchor->parameter);
    }
    if (line.alignment != LineAlignment::Free) {
      auto second = intersect(line.second, line.first.position, line.alignment);
      if (!second) return true;
      line.second = *second;
    }
    return distance(line.first.position, line.second.position) < 1e-8;
  });
}
void ConstrainedLineEditor::paint(QPainter& painter) const {
  painter.save();
  const double radius = 5 / view_.transform().m11();
  auto draw = [&](QPointF a, QPointF b, bool selected) {
    QPen backing{Qt::white}; backing.setCosmetic(true); backing.setWidthF(selected ? 7 : 5);
    painter.setPen(backing); painter.drawLine(a, b);
    QPen pen{selected ? QColor{255, 65, 0} : QColor{180, 0, 210}};
    pen.setCosmetic(true); pen.setWidthF(selected ? 4 : 2.5); painter.setPen(pen); painter.drawLine(a, b);
    if (enabled_) { painter.setBrush(Qt::white); painter.drawEllipse(a, radius, radius); painter.drawEllipse(b, radius, radius); }
  };
  for (int i = 0; i < static_cast<int>(lines_.size()); ++i)
    draw(lines_[i].first.position, lines_[i].second.position, i == selected_);
  if(enabled_&&verticalPreview_)draw(verticalPreview_->first.position,verticalPreview_->second.position,false);
  if (enabled_ && first_ && hover_) draw(first_->position, hover_->position, false);
  if (enabled_ && (hover_ || first_)) {
    QPen pen{QColor{0, 110, 30}}; pen.setCosmetic(true); pen.setWidthF(2);
    painter.setPen(pen); painter.setBrush(QColor{100, 255, 120});
    if (first_) painter.drawEllipse(first_->position, radius, radius);
    if (hover_) painter.drawEllipse(hover_->position, radius, radius);
  }
  painter.restore();
}
}
