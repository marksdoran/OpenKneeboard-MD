/*
 * OpenKneeboard
 *
 * Copyright (C) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#pragma once

#include <OpenKneeboard/CursorEvent.h>
#include <OpenKneeboard/DXResources.h>
#include <OpenKneeboard/Events.h>

namespace OpenKneeboard {

class KneeboardState;

class DoodleRenderer final {
 public:
  DoodleRenderer(const DXResources&, KneeboardState*);
  ~DoodleRenderer();

  void Render(
    ID2D1DeviceContext*,
    uint16_t pageIndex,
    const D2D1_RECT_F& targetRect);
  void PostCursorEvent(
    EventContext,
    const CursorEvent&,
    uint16_t pageIndex,
    const D2D1_SIZE_U& nativePageSize);

  void Clear();
  void ClearPage(uint16_t pageIndex);

  Event<> evNeedsRepaintEvent;

 private:
  DXResources mDXR;
  KneeboardState* mKneeboard;

  winrt::com_ptr<ID2D1SolidColorBrush> mBrush;
  winrt::com_ptr<ID2D1SolidColorBrush> mEraser;

  struct Drawing {
    winrt::com_ptr<IDXGISurface> mSurface;
    winrt::com_ptr<ID2D1Bitmap1> mBitmap;
    float mScale {-1.0f};
    std::vector<CursorEvent> mBufferedEvents;
    bool mHaveCursor {false};
    D2D1_POINT_2F mCursorPoint;
    D2D1_SIZE_U mNativeSize {0, 0};
  };
  winrt::com_ptr<ID2D1DeviceContext> mDrawingContext;
  std::mutex mBufferedEventsMutex;
  std::vector<Drawing> mDrawings;

  ID2D1Bitmap* GetDrawingSurface(uint16_t index);

  void FlushCursorEvents();
};

}// namespace OpenKneeboard