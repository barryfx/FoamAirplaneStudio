#include "gui/OcctViewport.h"
#include <QToolBar>

#include <AIS_TexturedShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Pln.hxx>
#include <cstring>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <Graphic3d_CLight.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Graphic3d_TypeOfLightSource.hxx>
#include <Graphic3d_TypeOfShadingModel.hxx>
#include <Graphic3d_TransModeFlags.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Prs3d_LineAspect.hxx>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>
#include <gp_Trsf.hxx>
#include <Quantity_Color.hxx>
#include <V3d_TypeOfVisualization.hxx>
#if defined(_WIN32)
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <NCollection_Vec2.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>

namespace designrc::gui {
namespace {

constexpr int kGizmoScreenOffset = 112;

} // namespace

OcctViewport::OcctViewport(QWidget* parent) : QWidget{parent} {
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_PaintOnScreen);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setMinimumSize(640, 480);
  setStyleSheet("background: white;");
}

QPaintEngine* OcctViewport::paintEngine() const { return nullptr; }

void OcctViewport::setViewActions(const QList<QAction*>& actions) {
  if(!viewControls_) {
    viewControls_=new QToolBar{this};
    viewControls_->setObjectName("viewportViewControls");
    // OCCT paints directly into this native viewport. A native child keeps Qt
    // controls above the OpenGL surface during redraws and camera movement.
    viewControls_->setAttribute(Qt::WA_NativeWindow);
    viewControls_->setMovable(false);viewControls_->setFloatable(false);
    viewControls_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    viewControls_->setStyleSheet("QToolBar { background: #f3f3f3; border: 1px solid #bcbcbc; padding: 3px; }");
  }
  viewControls_->clear();viewControls_->addActions(actions);
  positionViewControls();viewControls_->show();viewControls_->raise();
}
void OcctViewport::positionViewControls() {
  if(!viewControls_)return;
  const auto size=viewControls_->sizeHint();
  const int controlWidth=std::min(size.width(),std::max(1,width()-24));
  viewControls_->setGeometry((width()-controlWidth)/2,std::max(0,height()-size.height()-12),controlWidth,size.height());
}

void OcctViewport::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  initializeViewer();
  QTimer::singleShot(0, this, [this] {
    if (view_.IsNull()) return;
    view_->MustBeResized();
    view_->Invalidate();
    redraw();
  });
}

void OcctViewport::paintEvent(QPaintEvent*) {
  initializeViewer();
  redraw();
}

void OcctViewport::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  positionViewControls();
  if (!view_.IsNull()) view_->MustBeResized();
}

void OcctViewport::initializeViewer() {
  if (initialized_) return;
  initialized_ = true;

  const auto connection = Handle(Aspect_DisplayConnection){new Aspect_DisplayConnection};
  const auto driver = Handle(OpenGl_GraphicDriver){new OpenGl_GraphicDriver{connection}};
  viewer_ = Handle(V3d_Viewer){new V3d_Viewer{driver}};
  viewer_->SetDefaultTypeOfView(V3d_ORTHOGRAPHIC);

  // Symmetric upper/lower fill lights keep either side of the wing readable
  // while preserving equal illumination of the mirrored halves. Ambient fill
  // also keeps steep hinge/edge faces visible without a ray-tracing dependency.
  const auto ambient = Handle(Graphic3d_CLight){
      new Graphic3d_CLight{Graphic3d_TypeOfLightSource_Ambient}};
  ambient->SetIntensity(0.35F);
  viewer_->AddLight(ambient);
  viewer_->SetLightOn(ambient);

  const auto addDirectional = [this](const gp_Dir& direction) {
    const auto light = Handle(Graphic3d_CLight){
        new Graphic3d_CLight{Graphic3d_TypeOfLightSource_Directional}};
    light->SetDirection(direction);
    light->SetIntensity(0.42F);
    viewer_->AddLight(light);
    viewer_->SetLightOn(light);
  };
  addDirectional(gp_Dir{-0.35, -0.55, -1.0});
  addDirectional(gp_Dir{-0.35,  0.55, -1.0});
  addDirectional(gp_Dir{-0.35, -0.55,  1.0});
  addDirectional(gp_Dir{-0.35,  0.55,  1.0});

  context_ = Handle(AIS_InteractiveContext){new AIS_InteractiveContext{viewer_}};
  context_->SetDisplayMode(AIS_Shaded, false);
  view_ = viewer_->CreateView();
#if defined(_WIN32)
  const auto window = Handle(WNT_Window){
      new WNT_Window{reinterpret_cast<Aspect_Handle>(winId()), Quantity_NOC_WHITE}};
#else
  const auto window = Handle(Xw_Window){
      new Xw_Window{connection, static_cast<Aspect_Drawable>(winId())}};
#endif
  view_->SetWindow(window);
  view_->SetBackgroundColor(Quantity_NOC_WHITE);
  if (!window->IsMapped()) window->Map();

  view_->SetProj(V3d_XposYnegZpos);
  displayViewGizmo();
  if (!pendingWoodShape_.IsNull() || !pendingCarbonFiberShape_.IsNull() ||
      !pendingAluminumShape_.IsNull() || !pendingSteelShape_.IsNull() ||
      !pendingFiberglassShape_.IsNull())
    displayPendingShapes();
  view_->MustBeResized();
  redraw();
}

void OcctViewport::displayShape(const TopoDS_Shape& shape, bool fit) {
  clearAssemblyReference();
  pendingAssembly_.reset();
  foamAppearance_ = true;
  pendingWoodShape_ = shape;
  pendingCarbonFiberShape_.Nullify(); pendingAluminumShape_.Nullify();
  pendingSteelShape_.Nullify(); pendingFiberglassShape_.Nullify();
  if (context_.IsNull()) return;
  displayPendingShapes();
  if (fit) fitAll();
  else { view_->ZFitAll(); redraw(); }
}

void OcctViewport::displayMaterialShapes(const TopoDS_Shape& wood,
                                         const TopoDS_Shape& carbonFiber,
                                         const TopoDS_Shape& aluminum,
                                         const TopoDS_Shape& steel,
                                         const TopoDS_Shape& fiberglass) {
  clearAssemblyReference();
  pendingAssembly_.reset();
  foamAppearance_ = false;
  pendingWoodShape_ = wood;
  pendingCarbonFiberShape_ = carbonFiber;
  pendingAluminumShape_ = aluminum;
  pendingSteelShape_ = steel;
  pendingFiberglassShape_ = fiberglass;
  if (context_.IsNull()) return;
  displayPendingShapes();
  fitAll();
}

void OcctViewport::displayAssembly(const std::vector<TopoDS_Shape>& parts,int selected) {
  pendingAssembly_=parts;assemblySelected_=selected;foamAppearance_=true;
  if(context_.IsNull())initializeViewer();
  if(context_.IsNull())return;
  displayPendingShapes();view_->ZFitAll();redraw();
}
void OcctViewport::displayInspection(const std::vector<TopoDS_Shape>& parts) {
  clearAssemblyReference();displayAssembly(parts,-1);
}

void OcctViewport::clearAssemblyReference() {
  if(!context_.IsNull())for(const auto& object:referenceObjects_)context_->Remove(object,false);
  referenceObjects_.clear();assemblyReferencePages_.clear();
  setProperty("assemblyReferencePages",0);
}
void OcctViewport::setAssemblyReference(const ProjectReference& reference,const geometry::FuselageSideTransform& transform) {
  bool same=reference.toScale==assemblyReferenceToScale_&&reference.image.pages.size()==assemblyReferencePages_.size()
      &&transform.left==assemblyReferenceTransform_.left&&transform.verticalOrigin==assemblyReferenceTransform_.verticalOrigin
      &&transform.scale==assemblyReferenceTransform_.scale;
  for(std::size_t i=0;same&&i<reference.image.pages.size();++i)
    same=reference.image.pages[i].pixels.cacheKey()==assemblyReferencePages_[i].pixels.cacheKey()
      &&reference.image.pages[i].physicalSizeMm==assemblyReferencePages_[i].physicalSizeMm;
  if(same)return;
  clearAssemblyReference();
  assemblyReferencePages_=reference.image.pages;assemblyReferenceToScale_=reference.toScale;assemblyReferenceTransform_=transform;
  if(context_.IsNull())initializeViewer();
  double top=0;
  for(const auto& page:reference.image.pages) {
    const QSizeF size=reference.toScale&&page.physicalSizeMm?*page.physicalSizeMm:QSizeF{page.pixels.size()};
    if(page.pixels.isNull())continue;
    const auto rgba=page.pixels.convertToFormat(QImage::Format_RGBA8888);
    Handle(Image_PixMap) pixels=new Image_PixMap;
    if(!pixels->InitTrash(Image_Format_RGBA,rgba.width(),rgba.height()))continue;
    // OCCT textures consume bottom-up rows; Qt images use top-down rows.
    pixels->SetTopDown(false);
    for(int row=0;row<rgba.height();++row)
      std::memcpy(pixels->ChangeRow(row),rgba.constScanLine(row),static_cast<std::size_t>(rgba.width())*4);
    const double left=-transform.left*transform.scale;
    const double bottom=(transform.verticalOrigin-top-size.height())*transform.scale;
    // Plane U is +X and V is +Z, matching the side camera. Bottom layer draws
    // the reference behind all model geometry without affecting model depth.
    const gp_Pln plane{gp_Ax3{gp_Pnt{left,0,bottom},gp_Dir{0,-1,0},gp_Dir{1,0,0}}};
    const auto face=BRepBuilderAPI_MakeFace{plane,0,size.width()*transform.scale,0,size.height()*transform.scale}.Face();
    BRepMesh_IncrementalMesh mesh{face,.1};
    Handle(AIS_TexturedShape) object=new AIS_TexturedShape{face};
    object->SetTexturePixMap(pixels);object->SetTextureMapOn();object->SetTextureRepeat(false);
    object->DisableTextureModulate();object->Attributes()->SetFaceBoundaryDraw(false);
    object->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT,true);
    object->SetZLayer(Graphic3d_ZLayerId_BotOSD);
    context_->Display(object,3,-1,false);referenceObjects_.push_back(object);
    top+=size.height();
  }
  setProperty("assemblyReferencePages",static_cast<int>(referenceObjects_.size()));redraw();
}

void OcctViewport::displayPendingShapes() {
  for (const auto& displayed : displayedShapes_) context_->Remove(displayed, false);
  displayedShapes_.clear();

  enum class Appearance { Wood, CarbonFiber, Aluminum, Steel, Fiberglass };
  const auto display = [&](const TopoDS_Shape& shape, const Appearance appearance) {
    if (shape.IsNull() || !TopExp_Explorer{shape, TopAbs_FACE}.More()) return;
    auto object = Handle(AIS_Shape){new AIS_Shape{shape}};
    // Geometry providers triangulate their shapes before publication.
    object->Attributes()->SetAutoTriangulation(false);
    object->Attributes()->SetupOwnShadingAspect();
      if (appearance == Appearance::Wood && foamAppearance_) {
        object->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Phong, true);
        object->SetMaterial(Graphic3d_MaterialAspect{Graphic3d_NOM_PLASTIC});
        object->SetColor(Quantity_Color{0.83, 0.88, 0.94, Quantity_TOC_RGB});
        // Section seams are construction topology, not cuts in the foam body.
        object->Attributes()->SetFaceBoundaryDraw(false);
      } else if (appearance == Appearance::Wood) {
      object->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT, true);
      Graphic3d_MaterialAspect material{Graphic3d_NOM_PLASTIC};
      material.SetShininess(0.25F);
      object->SetMaterial(material);
      object->SetColor(Quantity_Color{212.0 / 255.0, 189.0 / 255.0,
                                      165.0 / 255.0, Quantity_TOC_RGB});
      object->Attributes()->SetFaceBoundaryDraw(true);
      object->Attributes()->SetFaceBoundaryAspect(
          Handle(Prs3d_LineAspect){new Prs3d_LineAspect{
              Quantity_Color{105.0 / 255.0, 80.0 / 255.0, 60.0 / 255.0,
                             Quantity_TOC_RGB},
              Aspect_TOL_SOLID, 1.0}});
    } else if (appearance == Appearance::CarbonFiber) {
      Graphic3d_MaterialAspect material{Graphic3d_NOM_PLASTIC};
      material.SetShininess(0.85F);
      object->SetMaterial(material);
      object->SetColor(Quantity_Color{0.015, 0.015, 0.015, Quantity_TOC_RGB});
    } else if (appearance == Appearance::Aluminum) {
      object->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT, true);
      Graphic3d_MaterialAspect material{Graphic3d_NOM_ALUMINIUM};
      material.SetShininess(0.9F);
      object->SetMaterial(material);
      object->SetColor(Quantity_Color{0.68, 0.70, 0.74, Quantity_TOC_RGB});
    } else if (appearance == Appearance::Steel) {
      object->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT, true);
      Graphic3d_MaterialAspect material{Graphic3d_NOM_STEEL};
      material.SetShininess(0.95F);
      object->SetMaterial(material);
      object->SetColor(Quantity_Color{0.90, 0.91, 0.93, Quantity_TOC_RGB});
    } else {
      object->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT, true);
      Graphic3d_MaterialAspect material{Graphic3d_NOM_PLASTIC};
      material.SetShininess(0.3F);
      object->SetMaterial(material);
      object->SetColor(Quantity_Color{0.96, 0.91, 0.72, Quantity_TOC_RGB});
    }
    // Selection is not currently used; skip its expensive face hierarchy.
    context_->Display(object, AIS_Shaded, -1, false);
    displayedShapes_.push_back(object);
  };
  if(pendingAssembly_) {
    const std::array<Quantity_Color,4> colors{
      Quantity_Color{.83,.88,.94,Quantity_TOC_RGB},Quantity_Color{.65,.8,.95,Quantity_TOC_RGB},
      Quantity_Color{.65,.88,.72,Quantity_TOC_RGB},Quantity_Color{.85,.73,.95,Quantity_TOC_RGB}};
    for(std::size_t i=0;i<pendingAssembly_->size();++i) {
      const auto previous=displayedShapes_.size();display((*pendingAssembly_)[i],Appearance::Wood);
      if(displayedShapes_.size()>previous) {
        auto object=displayedShapes_.back();
        object->SetColor(static_cast<int>(i)==assemblySelected_?Quantity_Color{1.,.55,.12,Quantity_TOC_RGB}:
            i<colors.size()?colors[i]:Quantity_Color{.85,.72,.5,Quantity_TOC_RGB});
        context_->Redisplay(object,false);
      }
    }
    context_->UpdateCurrentViewer();return;
  }
  display(pendingWoodShape_, Appearance::Wood);
  display(pendingCarbonFiberShape_, Appearance::CarbonFiber);
  display(pendingAluminumShape_, Appearance::Aluminum);
  display(pendingSteelShape_, Appearance::Steel);
  display(pendingFiberglassShape_, Appearance::Fiberglass);
  // Publish all material presentations before FitAll queries their combined
  // bounds. With several newly displayed AIS objects, deferred viewer updates
  // can otherwise leave the first automatic fit using incomplete extents.
  context_->UpdateCurrentViewer();
}

void OcctViewport::clearShape() {
  clearAssemblyReference();
  pendingAssembly_.reset();
  pendingWoodShape_.Nullify();
  pendingCarbonFiberShape_.Nullify();
  pendingAluminumShape_.Nullify();
  pendingSteelShape_.Nullify();
  pendingFiberglassShape_.Nullify();
  if (context_.IsNull()) return;
  for (const auto& displayed : displayedShapes_) context_->Remove(displayed, false);
  displayedShapes_.clear();
  redraw();
}

void OcctViewport::fitAll() {
  if (view_.IsNull()) return;
  if(pendingAssembly_) {
    Bnd_Box bounds;for(const auto& shape:*pendingAssembly_)if(!shape.IsNull())BRepBndLib::Add(shape,bounds);
    if(!bounds.IsVoid())view_->FitAll(bounds,0.05,false);
  } else view_->FitAll(0.05, false);
  view_->ZFitAll();
  redraw();
}
std::optional<CameraState> OcctViewport::cameraState() const {
  if(view_.IsNull()) return {};
  const auto camera=view_->Camera();
  const auto eye=camera->Eye(), center=camera->Center(); const auto up=camera->Up();
  return CameraState{{eye.X(),eye.Y(),eye.Z()},{center.X(),center.Y(),center.Z()},
      {up.X(),up.Y(),up.Z()},camera->Scale(),camera->FOVy(),static_cast<int>(camera->ProjectionType())};
}
void OcctViewport::resetCamera() {
  if(view_.IsNull())return;
  view_->SetCamera(new Graphic3d_Camera);
  view_->SetProj(V3d_XposYnegZpos);redraw();
}
void OcctViewport::restoreCamera(const std::optional<CameraState>& state) {
  if(!state) return;
  initializeViewer(); if(view_.IsNull()) return;
  const auto& s=*state; auto camera=view_->Camera();
  camera->SetProjectionType(static_cast<Graphic3d_Camera::Projection>(s.projection));
  camera->SetEyeAndCenter(gp_Pnt{s.eye[0],s.eye[1],s.eye[2]},gp_Pnt{s.center[0],s.center[1],s.center[2]});
  camera->SetUp(gp_Dir{s.up[0],s.up[1],s.up[2]});
  camera->SetScale(s.scale); camera->SetFOVy(s.fov); view_->ZFitAll(); redraw();
}

void OcctViewport::setCameraView(const CameraView cameraView) {
  initializeViewer();
  if (view_.IsNull()) return;
  switch (cameraView) {
    case CameraView::Reset:  view_->SetProj(V3d_XposYnegZpos); break;
    case CameraView::Top:
      view_->SetProj(V3d_Zpos);
      view_->SetUp(-1.0, 0.0, 0.0);
      break;
    case CameraView::Bottom:
      view_->SetProj(V3d_Zneg);
      view_->SetUp(-1.0, 0.0, 0.0);
      break;
    // The wing chord runs along +X, span along +/-Y, and vertical along +Z.
    // Front therefore looks aft from the leading edge, while left and right
    // look inward from their corresponding wing tips.
    case CameraView::Front:  view_->SetProj(V3d_Xneg); break;
    case CameraView::Back:   view_->SetProj(V3d_Xpos); break;
    case CameraView::Left:   view_->SetProj(V3d_Yneg); break;
    case CameraView::Right:  view_->SetProj(V3d_Ypos); break;
  }
  fitAll();
}

void OcctViewport::mousePressEvent(QMouseEvent* event) {
  lastMousePosition_ = (event->position()*devicePixelRatioF()).toPoint();
  if (!view_.IsNull() && event->button() == Qt::LeftButton) {
    orbiting_ = true;
    view_->StartRotation(lastMousePosition_.x(), lastMousePosition_.y());
    event->accept();
    return;
  }
  if (event->button() == Qt::RightButton) {
    panning_ = true;
    event->accept();
    return;
  }
  if (event->button() == Qt::MiddleButton) {
    event->accept();
    return;
  }
  QWidget::mousePressEvent(event);
}

void OcctViewport::mouseMoveEvent(QMouseEvent* event) {
  const QPoint position = (event->position()*devicePixelRatioF()).toPoint();
  if (!view_.IsNull() && orbiting_) {
    view_->Rotation(position.x(), position.y());
    redraw();
  } else if (!view_.IsNull() && panning_) {
    view_->Pan(position.x() - lastMousePosition_.x(), lastMousePosition_.y() - position.y());
    redraw();
  }
  lastMousePosition_ = position;
}

void OcctViewport::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) orbiting_ = false;
  if (event->button() == Qt::RightButton) panning_ = false;
  if (event->button() == Qt::MiddleButton) {
    event->accept();
    return;
  }
  QWidget::mouseReleaseEvent(event);
}

void OcctViewport::wheelEvent(QWheelEvent* event) {
  if (view_.IsNull() || event->angleDelta().y() == 0) return;
  const QPoint position = (event->position()*devicePixelRatioF()).toPoint();
  double bx,by,bz,ax,ay,az;
  view_->Convert(position.x(),position.y(),bx,by,bz);
  const double factor=std::pow(1.2,event->angleDelta().y()/120.);
  view_->Camera()->SetScale(std::clamp(view_->Camera()->Scale()/factor,1e-6,1e12));
  view_->Convert(position.x(),position.y(),ax,ay,az);
  // Preserve the point on the camera plane beneath the cursor at any rotation.
  gp_Trsf shift;shift.SetTranslation(gp_Vec{ax,ay,az}.Reversed()+gp_Vec{bx,by,bz});
  view_->Camera()->Transform(shift);view_->ZFitAll();
  redraw();
  event->accept();
}

void OcctViewport::redraw() {
  if (!view_.IsNull()) view_->Redraw();
}

void OcctViewport::displayViewGizmo() {
  if (context_.IsNull()) return;
  viewGizmoPersistence_ = Handle(Graphic3d_TransformPers){new Graphic3d_TransformPers{
      Graphic3d_TMF_TriedronPers, Aspect_TOTP_RIGHT_UPPER,
      NCollection_Vec2<int>{kGizmoScreenOffset, kGizmoScreenOffset}}};

  const auto prepare = [this](const Handle(AIS_Shape)& shape) {
    shape->SetTransformPersistence(viewGizmoPersistence_);
    shape->SetZLayer(Graphic3d_ZLayerId_Topmost);
  };
  const auto addDecoration = [this, &prepare](const TopoDS_Shape& shape,
                                             const Quantity_Color& color) {
    auto object = Handle(AIS_Shape){new AIS_Shape{shape}};
    object->SetColor(color);
    prepare(object);
    context_->Display(object, AIS_Shaded, -1, false);
    viewGizmoObjects_.push_back(object);
  };
  const auto addAxis = [&addDecoration](const gp_Dir& direction, const Quantity_Color& color) {
    const gp_XYZ origin{0.0, 0.0, 0.0};
    const gp_XYZ vector{direction.X(), direction.Y(), direction.Z()};
    addDecoration(BRepPrimAPI_MakeCylinder(
        gp_Ax2{gp_Pnt{origin}, direction}, 3.5, 54.0).Shape(), color);
    addDecoration(BRepPrimAPI_MakeCone(
        gp_Ax2{gp_Pnt{origin + vector * 54.0}, direction},
        8.0, 0.0, 10.0).Shape(), color);
  };
  addAxis(gp_Dir{1.0, 0.0, 0.0}, Quantity_Color{0.82, 0.12, 0.08, Quantity_TOC_RGB});
  addAxis(gp_Dir{0.0, 1.0, 0.0}, Quantity_Color{0.10, 0.55, 0.18, Quantity_TOC_RGB});
  addAxis(gp_Dir{0.0, 0.0, 1.0}, Quantity_Color{0.12, 0.32, 0.90, Quantity_TOC_RGB});
  context_->UpdateCurrentViewer();
}

} // namespace designrc::gui
