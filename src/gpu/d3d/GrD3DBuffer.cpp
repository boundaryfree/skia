/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/d3d/GrD3DBuffer.h"

#include "src/gpu/d3d/GrD3DGpu.h"
#include "src/gpu/d3d/GrD3DUtil.h"

#ifdef SK_DEBUG
#define VALIDATE() this->validate()
#else
#define VALIDATE() do {} while(false)
#endif

static gr_cp<ID3D12Resource> make_d3d_buffer(GrD3DGpu* gpu,
                                              size_t size,
                                              GrGpuBufferType intendedType,
                                              GrAccessPattern accessPattern,
                                              D3D12_RESOURCE_STATES* resourceState) {
    D3D12_HEAP_TYPE heapType;
    if (accessPattern == kStatic_GrAccessPattern) {
        SkASSERT(intendedType != GrGpuBufferType::kXferCpuToGpu &&
                 intendedType != GrGpuBufferType::kXferGpuToCpu);
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        // Needs to be transitioned to appropriate state to be read in shader
        *resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    } else {
        if (intendedType == GrGpuBufferType::kXferGpuToCpu) {
            heapType = D3D12_HEAP_TYPE_READBACK;
            // Cannot be changed
            *resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
        } else {
            heapType = D3D12_HEAP_TYPE_UPLOAD;
            // Cannot be changed
            // Includes VERTEX_AND_CONSTANT_BUFFER, INDEX_BUFFER, INDIRECT_ARGUMENT, and COPY_SOURCE
            *resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
    }

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = heapType;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;  // default alignment
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0; // Doesn't apply to buffers
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* resource;
    HRESULT hr = gpu->device()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            *resourceState,
            nullptr,
            IID_PPV_ARGS(&resource));
    if (!SUCCEEDED(hr)) {
        return nullptr;
    }

    return gr_cp<ID3D12Resource>(resource);
}

sk_sp<GrD3DBuffer> GrD3DBuffer::Make(GrD3DGpu* gpu, size_t size, GrGpuBufferType intendedType,
                                     GrAccessPattern accessPattern) {
    SkASSERT(!gpu->protectedContext() || (accessPattern != kStatic_GrAccessPattern));
    D3D12_RESOURCE_STATES resourceState;


    gr_cp<ID3D12Resource> resource = make_d3d_buffer(gpu, size, intendedType, accessPattern,
                                                     &resourceState);
    if (!resource) {
        return nullptr;
    }

    return sk_sp<GrD3DBuffer>(new GrD3DBuffer(gpu, size, intendedType, accessPattern,
                                              std::move(resource), resourceState));
}

GrD3DBuffer::GrD3DBuffer(GrD3DGpu* gpu, size_t size, GrGpuBufferType intendedType,
                         GrAccessPattern accessPattern, gr_cp<ID3D12Resource> bufferResource,
                         D3D12_RESOURCE_STATES resourceState)
    : INHERITED(gpu, size, intendedType, accessPattern)
    , fResourceState(resourceState)
    , fD3DResource(std::move(bufferResource)) {
    this->registerWithCache(SkBudgeted::kYes);

    // TODO: persistently map UPLOAD resources?

    VALIDATE();
}

void GrD3DBuffer::setResourceState(const GrD3DGpu* gpu,
                                   D3D12_RESOURCE_STATES newResourceState) {
    if (newResourceState == fResourceState ||
        // GENERIC_READ encapsulates a lot of different read states
        (fResourceState == D3D12_RESOURCE_STATE_GENERIC_READ &&
         SkToBool(newResourceState | fResourceState))) {
        return;
    }

    D3D12_RESOURCE_TRANSITION_BARRIER barrier = {};
    barrier.pResource = this->d3dResource();
    barrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.StateBefore = fResourceState;
    barrier.StateAfter = newResourceState;

    gpu->addBufferResourceBarriers(this, 1, &barrier);

    fResourceState = newResourceState;
}

void GrD3DBuffer::onRelease() {
    if (!this->wasDestroyed()) {
        VALIDATE();
        // Note: we intentionally don't release the d3d resource here since it may still be in use
        // by the gpu and a call to GrContext::release could get us in here.
        fMapPtr = nullptr;
        VALIDATE();
    }
    INHERITED::onRelease();
}

void GrD3DBuffer::onAbandon() {
    if (!this->wasDestroyed()) {
        VALIDATE();
        // Note: we intentionally don't release the d3d resource here since it may still be in use
        // by the gpu and a call to GrContext::abandon could get us in here.
        fMapPtr = nullptr;
        VALIDATE();
    }
    INHERITED::onAbandon();
}

void GrD3DBuffer::onMap() {
    this->internalMap(this->size());
}

void GrD3DBuffer::onUnmap() {
    this->internalUnmap(this->size());
}

bool GrD3DBuffer::onUpdateData(const void* src, size_t size) {
    SkASSERT(src);
    if (size > this->size()) {
        return false;
    }
    if (!fD3DResource) {
        return false;
    }

    this->internalMap(size);
    if (!fMapPtr) {
        return false;
    }
    if (this->accessPattern() == kStatic_GrAccessPattern) {
        // We should never call this method on static buffers in protected contexts.
        SkASSERT(!this->getD3DGpu()->protectedContext());
        //*** any alignment restrictions?
    }
    memcpy(fMapPtr, src, size);
    this->internalUnmap(size);

    return true;
}

void GrD3DBuffer::internalMap(size_t size) {
    // TODO: if UPLOAD heap type, could be persistently mapped (i.e., this would be a no-op)
    if (this->wasDestroyed()) {
        return;
    }
    SkASSERT(fD3DResource);
    SkASSERT(!this->isMapped());
    SkASSERT(this->size() >= size);

    VALIDATE();

    if (this->accessPattern() == kStatic_GrAccessPattern) {
        SkASSERT(!fStagingBuffer);
        GrStagingBufferManager::Slice slice =
                this->getD3DGpu()->stagingBufferManager()->allocateStagingBufferSlice(size);
        if (!slice.fBuffer) {
            return;
        }
        fStagingBuffer = static_cast<const GrD3DBuffer*>(slice.fBuffer)->d3dResource();
        fStagingOffset = slice.fOffset;
        fMapPtr = slice.fOffsetMapPtr;
    } else {
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = size;
        fD3DResource->Map(0, &range, &fMapPtr);
    }

    VALIDATE();
}

void GrD3DBuffer::internalUnmap(size_t size) {
    // TODO: if UPLOAD heap type, could be persistently mapped (i.e., this would be a no-op)
    if (this->wasDestroyed()) {
        return;
    }
    SkASSERT(fD3DResource);
    SkASSERT(this->isMapped());
    VALIDATE();

#ifdef SK_BUILD_FOR_MAC
    // In both cases the size needs to be 4-byte aligned on Mac
    sizeInBytes = SkAlign4(sizeInBytes);
#endif
    if (this->accessPattern() == kStatic_GrAccessPattern) {
        SkASSERT(fStagingBuffer);
        this->setResourceState(this->getD3DGpu(), D3D12_RESOURCE_STATE_COPY_DEST);
        this->getD3DGpu()->currentCommandList()->copyBufferToBuffer(
                sk_ref_sp<GrD3DBuffer>(this), 0, fStagingBuffer, fStagingOffset, size);
        fStagingBuffer = nullptr;
    } else {
        D3D12_RANGE range;
        range.Begin = 0;
        // For READBACK heaps, unmap requires an empty range
        range.End = fResourceState == D3D12_RESOURCE_STATE_COPY_DEST ? 0 : size;
        SkASSERT(this->size() >= size);
        fD3DResource->Unmap(0, &range);
    }

    fMapPtr = nullptr;

    VALIDATE();
}

#ifdef SK_DEBUG
void GrD3DBuffer::validate() const {
    SkASSERT(this->intendedType() == GrGpuBufferType::kVertex ||
             this->intendedType() == GrGpuBufferType::kIndex ||
             this->intendedType() == GrGpuBufferType::kDrawIndirect ||
             this->intendedType() == GrGpuBufferType::kXferCpuToGpu ||
             this->intendedType() == GrGpuBufferType::kXferGpuToCpu);
}
#endif