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

#include <OpenKneeboard/Tab.h>
#include <dxgi1_2.h>
#include <shims/winrt.h>

namespace OpenKneeboard {

struct DXResources;

class DrawableTab : public virtual Tab {
 public:
  DrawableTab(const DXResources&);
  virtual ~DrawableTab();

  virtual void OnCursorEvent(const CursorEvent&, uint16_t pageIndex) override;

  virtual void RenderPage(
    uint16_t pageIndex,
    const winrt::com_ptr<ID2D1RenderTarget>& target,
    const D2D1_RECT_F& rect) override final;

 protected:
  void ClearDrawings();

  virtual void RenderPageContent(
    uint16_t pageIndex,
    const winrt::com_ptr<ID2D1RenderTarget>& target,
    const D2D1_RECT_F& rect)
    = 0;

 private:
  struct Impl;
  std::shared_ptr<Impl> p;
};

}// namespace OpenKneeboard