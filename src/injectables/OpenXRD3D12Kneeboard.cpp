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

#include "OpenXRD3D12Kneeboard.h"

#include <OpenKneeboard/D3D11.h>
#include <OpenKneeboard/config.h>
#include <OpenKneeboard/dprint.h>
#include <OpenKneeboard/tracing.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <shims/winrt/base.h>

#include <string>

#include "OpenXRD3D11Kneeboard.h"
#include "OpenXRNext.h"

#define XR_USE_GRAPHICS_API_D3D12
#include <openxr/openxr_platform.h>

namespace OpenKneeboard {

OpenXRD3D12Kneeboard::OpenXRD3D12Kneeboard(
  XrSession session,
  OpenXRRuntimeID runtimeID,
  const std::shared_ptr<OpenXRNext>& next,
  const XrGraphicsBindingD3D12KHR& binding)
  : OpenXRKneeboard(session, runtimeID, next) {
  mDeviceResources.mDevice12.copy_from(binding.device);
  mDeviceResources.mCommandQueue12.copy_from(binding.queue);

  dprintf("{}", __FUNCTION__);
  TraceLoggingWrite(gTraceProvider, "OpenXRD3D12Kneeboard()");

  UINT flags = 0;
#ifdef DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D11On12CreateDevice(
    mDeviceResources.mDevice12.get(),
    flags,
    nullptr,
    0,
    nullptr,
    0,
    1,
    mDeviceResources.mDevice11.put(),
    mDeviceResources.mContext11.put(),
    nullptr);
  mDeviceResources.m11on12 = mDeviceResources.mDevice11.as<ID3D11On12Device2>();
}

OpenXRD3D12Kneeboard::~OpenXRD3D12Kneeboard() {
  TraceLoggingWrite(gTraceProvider, "~OpenXRD3D12Kneeboard()");
}

bool OpenXRD3D12Kneeboard::ConfigurationsAreCompatible(
  const VRRenderConfig& initial,
  const VRRenderConfig& current) const {
  if (!IsVarjoRuntime()) {
    return true;
  }

  return initial.mQuirks.mVarjo_OpenXR_D3D12_DoubleBuffer
    == current.mQuirks.mVarjo_OpenXR_D3D12_DoubleBuffer;
}

XrSwapchain OpenXRD3D12Kneeboard::CreateSwapChain(
  XrSession session,
  const VRRenderConfig& vrc,
  uint8_t layerIndex) {
  dprintf("{}", __FUNCTION__);

  XrSwapchainCreateInfo swapchainInfo {
    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
    .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
    .format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    .sampleCount = 1,
    .width = TextureWidth,
    .height = TextureHeight,
    .faceCount = 1,
    .arraySize = 1,
    .mipCount = 1,
  };

  auto oxr = this->GetOpenXR();

  XrSwapchain swapchain {nullptr};

  auto nextResult = oxr->xrCreateSwapchain(session, &swapchainInfo, &swapchain);
  if (nextResult != XR_SUCCESS) {
    dprintf("Failed to create swapchain: {}", nextResult);
    return nullptr;
  }

  uint32_t imageCount = 0;
  nextResult
    = oxr->xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
  if (imageCount == 0 || nextResult != XR_SUCCESS) {
    dprintf("No images in swapchain: {}", nextResult);
    return nullptr;
  }

  dprintf("{} images in swapchain", imageCount);

  bool doubleBuffer = false;
  if (IsVarjoRuntime() && vrc.mQuirks.mVarjo_OpenXR_D3D12_DoubleBuffer) {
    dprint("Enabling double-buffering for Varjo D3D11on12 quirk");
    doubleBuffer = true;
  }

  std::vector<XrSwapchainImageD3D12KHR> images;
  images.resize(
    imageCount,
    XrSwapchainImageD3D12KHR {
      .type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR,
    });

  nextResult = oxr->xrEnumerateSwapchainImages(
    swapchain,
    imageCount,
    &imageCount,
    reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data()));
  if (nextResult != XR_SUCCESS) {
    dprintf("Failed to enumerate images in swapchain: {}", nextResult);
    oxr->xrDestroySwapchain(swapchain);
    return nullptr;
  }

  if (images.at(0).type != XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
    dprint("Swap chain is not a D3D12 swapchain");
    OPENKNEEBOARD_BREAK;
    oxr->xrDestroySwapchain(swapchain);
    return nullptr;
  }

  mRenderTargetViews.at(layerIndex).resize(imageCount);

  for (size_t i = 0; i < imageCount; ++i) {
#ifdef DEBUG
    if (images.at(i).type != XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
      OPENKNEEBOARD_BREAK;
    }
#endif
    winrt::com_ptr<ID3D12Resource> texture12;
    texture12.copy_from(images.at(i).texture);
    mRenderTargetViews.at(layerIndex).at(i)
      = std::static_pointer_cast<D3D11::IRenderTargetViewFactory>(
        std::make_shared<D3D11On12::RenderTargetViewFactory>(
          mDeviceResources,
          texture12,
          doubleBuffer ? D3D11On12::Flags::DoubleBuffer
                       : D3D11On12::Flags::None));
  }
  dprintf(
    "Created {} 11on12 RenderTargetViews for layer {}", imageCount, layerIndex);

  return swapchain;
}

bool OpenXRD3D12Kneeboard::Render(
  XrSwapchain swapchain,
  const SHM::Snapshot& snapshot,
  uint8_t layerIndex,
  const VRKneeboard::RenderParameters& renderParameters) {
  if (!OpenXRD3D11Kneeboard::Render(
        this->GetOpenXR(),
        mDeviceResources.mDevice11.get(),
        mRenderTargetViews.at(layerIndex),
        swapchain,
        snapshot,
        layerIndex,
        renderParameters)) {
    return false;
  }
  mDeviceResources.mContext11->Flush();
  return true;
}

winrt::com_ptr<ID3D11Device> OpenXRD3D12Kneeboard::GetD3D11Device() const {
  return mDeviceResources.mDevice11;
}

}// namespace OpenKneeboard
