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

#include <OpenKneeboard/DXResources.h>
#include <OpenKneeboard/IPageSource.h>
#include <shims/winrt/base.h>

namespace OpenKneeboard {

class HWNDPageSource final : public virtual IPageSource {
 public:
  HWNDPageSource() = delete;
  HWNDPageSource(const DXResources&, HWND window);
  virtual ~HWNDPageSource();

  virtual PageIndex GetPageCount() const final override;
  virtual D2D1_SIZE_U GetNativeContentSize(PageIndex pageIndex) final override;

  virtual void RenderPage(
    ID2D1DeviceContext*,
    PageIndex pageIndex,
    const D2D1_RECT_F& rect) final override;

 private:
  DXResources mDXR;

  std::mutex mMutex;
};

}// namespace OpenKneeboard