// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CJX_ROOTELEMENT_H_
#define FXJS_XFA_CJX_ROOTELEMENT_H_

#include "fxjs/xfa/cjx_textnode.h"
#include "fxjs/xfa/jse_define.h"

class CXFA_RootElement;

class CJX_RootElement final : public CJX_TextNode {
 public:
  explicit CJX_RootElement(CXFA_RootElement* node);
  ~CJX_RootElement() override;
};

#endif  // FXJS_XFA_CJX_ROOTELEMENT_H_
