/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "example/HelloWorld.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSurface.h"
//#include "include/core/SkVertices.h"
#include "include/effects/SkGradientShader.h"
#include "include/docs/SkPDFDocument.h"
#include "include/effects/Sk2DPathEffect.h"

#include <chrono>

using namespace sk_app;


//
// Here is an example of using Skia’s PDF backend (SkPDF) via the SkDocument
// and SkCanvas APIs.
void WritePDF(SkWStream* outputStream,
              const char* documentTitle,
              void (*writePage)(SkCanvas*, int page),
              int numberOfPages,
              SkSize pageSize) {
    SkPDF::Metadata metadata;
    metadata.fTitle = documentTitle;
    metadata.fCreator = "Example WritePDF() Function";
    metadata.fCreation = {0, 2019, 1, 4, 31, 12, 34, 56};
    metadata.fModified = {0, 2019, 1, 4, 31, 12, 34, 56};
    auto pdfDocument = SkPDF::MakeDocument(outputStream, metadata);
    SkASSERT(pdfDocument);
    for (int page = 0; page < numberOfPages; ++page) {
        SkCanvas* pageCanvas = pdfDocument->beginPage(pageSize.width(),
                                                      pageSize.height());
        writePage(pageCanvas, page);
        pdfDocument->endPage();
    }
    pdfDocument->close();
}

// Print binary data to stdout as hex.
void print_data(const SkData* data, const char* name) {
    if (data) {
        SkDebugf("\nxxd -r -p > %s << EOF", name);
        size_t s = data->size();
        const uint8_t* d = data->bytes();
        for (size_t i = 0; i < s; ++i) {
            if (i % 40 == 0) { SkDebugf("\n"); }
            SkDebugf("%02x", d[i]);
        }
        SkDebugf("\nEOF\n\n");
    }
}

// example function that draws on a SkCanvas.
void write_page(SkCanvas* canvas, int) {
    const SkScalar R = 115.2f, C = 128.0f;
    SkPath path;
    path.moveTo(C + R, C);
    for (int i = 1; i < 8; ++i) {
        SkScalar a = 2.6927937f * i;
        path.lineTo(C + R * cos(a), C + R * sin(a));
    }
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    canvas->drawPath(path, paint);
}

void draw(SkCanvas*) {
    constexpr SkSize ansiLetterSize{8.5f * 72, 11.0f * 72};
    SkDynamicMemoryWStream buffer;
    WritePDF(&buffer, "SkPDF Example", &write_page, 1, ansiLetterSize);
    sk_sp<SkData> pdfData = buffer.detachAsData();
    print_data(pdfData.get(), "skpdf_example.pdf");
}


Application* Application::Create(int argc, char** argv, void* platformData) {
    return new HelloWorld(argc, argv, platformData);
}

HelloWorld::HelloWorld(int argc, char** argv, void* platformData)
        : fBackendType(Window::kNativeGL_BackendType)
        , fRotationAngle(0), fZoomFactor(1.0), needRefresh(true), transX(0), transY(0), isPan(false) {
    SkGraphics::Init();

    fWindow = Window::CreateNativeWindow(platformData);
    fWindow->setRequestedDisplayParams(DisplayParams());

    // register callbacks
    fWindow->pushLayer(this);

    fWindow->attach(fBackendType);
}

HelloWorld::~HelloWorld() {
    fWindow->detach();
    delete fWindow;
}

void HelloWorld::updateTitle() {
    if (!fWindow || fWindow->sampleCount() <= 1) {
        return;
    }

    SkString title("Hello World ");
    title.append(Window::kRaster_BackendType == fBackendType ? "Raster" : "OpenGL");
    fWindow->setTitle(title.c_str());
}

void HelloWorld::onBackendCreated() {
    this->updateTitle();
    fWindow->show();
    fWindow->inval();
}

void HelloWorld::drawByVertices(SkSurface * surface) {
//    auto canvas = surface->getCanvas();
//    SkPoint positions[] = {{0, 0}, {20, 20}}
//    sk_sp<SkVertices> vertices = SkVertices::MakeCopy();
}

void HelloWorld::onPaint(SkSurface* surface) {
    if (!needRefresh) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto canvas = surface->getCanvas();
    
    canvas->save();

    // Clear background
    canvas->clear(SK_ColorWHITE);

    
//    paint.setStrokeWidth(2);
//    paint.setStyle(SkPaint::kStroke_Style);

    // Draw a rectangle with red paint
    
    canvas->scale(fZoomFactor, fZoomFactor);
    canvas->translate(transX, transY);
    int w = surface->width() / 100, h = surface->height() / 100;
    SkRect rect = SkRect::MakeXYWH(0, 0, 20, 20);
    float colorFactor = 1;
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
//            SkRect rect = SkRect::MakeXYWH((10 + w) * j, (10 + h) * i, w, h);
            SkPaint paint;
            paint.setColor({1* colorFactor, 0, 0, 1});
            
            SkMatrix lattice;
            lattice.setScale(2.0f, 2.0f);
            lattice.preRotate(30.0f);
            paint.setPathEffect(SkLine2DPathEffect::Make(0.0f, lattice));
            paint.setAntiAlias(true);
            
            canvas->save();
            canvas->translate((10 + w) * j, (10 + h) * i);
            canvas->drawRect(rect, paint);
//            canvas->drawLine(0, 0, 20, 20, paint);
            canvas->restore();
            colorFactor *= 0.9;
            if (colorFactor < 0.0001) {
                colorFactor = 1;
            }
        }
    }
    
    canvas->restore();

    // Set up a linear gradient and draw a circle
//    {
//        SkPoint linearPoints[] = { { 0, 0 }, { 300, 300 } };
//        SkColor linearColors[] = { SK_ColorGREEN, SK_ColorBLACK };
//        paint.setShader(SkGradientShader::MakeLinear(linearPoints, linearColors, nullptr, 2,
//                                                     SkTileMode::kMirror));
//        paint.setAntiAlias(true);
//
//        canvas->drawCircle(200, 200, 64, paint);
//
//        // Detach shader
//        paint.setShader(nullptr);
//    }
//
//    // Draw a message with a nice black paint
//    SkFont font;
//    font.setSubpixel(true);
//    font.setSize(20);
//    paint.setColor(SK_ColorBLACK);
//
//    canvas->save();
//    static const char message[] = "Hello World";
//
//    // Translate and rotate
//    canvas->translate(300, 300);
//    fRotationAngle += 0.2f;
//    if (fRotationAngle > 360) {
//        fRotationAngle -= 360;
//    }
//    canvas->rotate(fRotationAngle);
//
//    // Draw the text
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> costTime = endTime - startTime;
    auto cost = costTime.count();
    
    char message[100];
    sprintf(message, "cost: %.2f", cost);
    
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    SkFont font;
    font.setSubpixel(true);
    font.setSize(20);
    paint.setColor(SK_ColorBLACK);
    canvas->drawSimpleText(message, strlen(message), SkTextEncoding::kUTF8, 20, 20, font, paint);

    canvas->restore();
    
    SkDebugf("cost time: %f\n", cost);
    
//    transX = 0;
//    transY = 0;
//    fZoomFactor = 1;
    
    needRefresh = false;
}

void HelloWorld::onIdle() {
    // Just re-paint continously
    if (!needRefresh) {
        return;
    }
    fWindow->inval();
}

bool HelloWorld::onChar(SkUnichar c, skui::ModifierKey modifiers) {
    if (' ' == c) {
        fBackendType = Window::kRaster_BackendType == fBackendType ? Window::kNativeGL_BackendType
                                                                   : Window::kRaster_BackendType;
        SkDebugf("backend type: %d, GL backend type: %d\n", fBackendType, Window::kNativeGL_BackendType);
        fWindow->detach();
        fWindow->attach(fBackendType);
    }
    else if ('p' == c) {
        draw(nullptr);
    }
    return true;
}

bool HelloWorld::onMouseWheel(float delta, skui::ModifierKey key) {
    if (delta > 0) {
        fZoomFactor *= 1.01;
        needRefresh = true;
    }
    else if (delta < 0) {
        fZoomFactor *= 0.99;
        needRefresh = true;
    }
    
    return true;
}

bool HelloWorld::onMouse(int x, int y, skui::InputState state, skui::ModifierKey key) {
    if (state == skui::InputState::kDown) {
        isPan = true;
        prevX = x;
        prevY = y;
        return true;
    }
    else if (state == skui::InputState::kUp) {
        isPan = false;
        return true;
    }
    else if (state == skui::InputState::kMove) {
        if (!isPan) {
            return true;
        }
        
        transX += (x - prevX) * 1;
        transY += (y - prevY) * 1;
        
        prevX = x;
        prevY = y;
        needRefresh = true;
        return true;
    }
    return true;
}

