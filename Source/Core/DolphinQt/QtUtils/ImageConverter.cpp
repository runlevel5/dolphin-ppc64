// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/QtUtils/ImageConverter.h"

#include <QPixmap>

#include "UICommon/GameFile.h"

QPixmap ToQPixmap(const UICommon::GameBanner& banner)
{
  // The banner's pixels are ARGB words in the host's order, which is what Format_ARGB32 is.
  const auto* ptr = reinterpret_cast<const uchar*>(banner.buffer.data());
  QImage image(ptr, banner.width, banner.height, QImage::Format_ARGB32);
  return QPixmap::fromImage(std::move(image));
}
