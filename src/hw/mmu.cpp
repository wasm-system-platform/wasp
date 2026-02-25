#include "hw/mmu.hpp"
#include "runtime/instance.hpp"

Errno MemoryManagementUnit::createTable(uint32_t* ptable_idx) {
    *ptable_idx = tables_.size();
    tables_.emplace_back();
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::loadTable(uint32_t ptable_idx) {
    if (ptable_idx >= tables_.size()) {
        return Errno::INVALID_ARGUMENT;
    }

    active_idx_ = ptable_idx;
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::mapPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t phys_addr, uint8_t* real_addr, int prot) {
    if (ptable_idx >= tables_.size()) {
        return Errno::INVALID_ARGUMENT;
    }

    return tables_[ptable_idx].mapPage(virt_addr, phys_addr, real_addr, prot);
}

Errno MemoryManagementUnit::unmapPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t* phys_addr, int* prot) {
    if (ptable_idx >= tables_.size()) {
        return Errno::INVALID_ARGUMENT;
    }

    return tables_[ptable_idx].unmapPage(virt_addr, phys_addr, prot);
}

Errno MemoryManagementUnit::getPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t* phys_addr, int* prot) {
    if (ptable_idx >= tables_.size()) {
        return Errno::INVALID_ARGUMENT;
    }

    return tables_[ptable_idx].getPage(virt_addr, phys_addr, prot);
}

Errno MemoryManagementUnit::read(uint32_t va, std::vector<uint8_t>& dst) {
    uint8_t* pa;
    for (size_t i = 0; i < dst.size(); i++) {
        Errno err = tables_[active_idx_].translate(va + i, &pa, READ);
        if (err != Errno::SUCCESS) {
            return err;
        }
    }

    for (size_t i = 0; i < dst.size(); i++) {
        tables_[active_idx_].translate(va + i, &pa, READ);
        dst[i] = *pa;
    }

    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::readI8(uint32_t va, int8_t* value) {
    uint8_t* pa;
    Errno err = tables_[active_idx_].translate(va, &pa, READ);
    if (err != Errno::SUCCESS) {
        return err;
    }

    *value = static_cast<int8_t>(*pa);
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::readI16(uint32_t va, int16_t* value) {
    uint16_t result = 0;

    for (int i = 0; i < sizeof(int16_t); ++i) {
        uint8_t* pa;
        Errno err = tables_[active_idx_].translate(va + i, &pa, READ);
        if (err != Errno::SUCCESS) {
            return err;
        }

        result |= static_cast<uint16_t>(*pa) << (i * 8);
    }

    *value = static_cast<int16_t>(result);
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::readI32(uint32_t va, int32_t* value) {
    uint32_t result = 0;

    for (int i = 0; i < sizeof(int32_t); ++i) {
        uint8_t* pa;
        Errno err = tables_[active_idx_].translate(va + i, &pa, READ);
        if (err != Errno::SUCCESS) {
            return err;
        }

        result |= static_cast<uint32_t>(*pa) << (i * 8);
    }

    *value = static_cast<int32_t>(result);
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::readI64(uint32_t va, int64_t* value) {
    uint64_t result = 0;

    for (int i = 0; i < sizeof(int64_t); ++i) {
        uint8_t* pa;
        Errno err = tables_[active_idx_].translate(va + i, &pa, READ);
        if (err != Errno::SUCCESS) {
            return err;
        }

        result |= static_cast<uint64_t>(*pa) << (i * 8);
    }

    *value = static_cast<int64_t>(result);
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::write(uint32_t va, const std::vector<uint8_t>& src) {
    uint8_t* pa;
    for (size_t i = 0; i < src.size(); i++) {
        Errno err = tables_[active_idx_].translate(va + i, &pa, WRITE);
        if (err != Errno::SUCCESS) {
            return err;
        }
    }

    for (size_t i = 0; i < src.size(); i++) {
        tables_[active_idx_].translate(va + i, &pa, WRITE);
        *pa = src[i];
    }

    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::writeI8(uint32_t va, int8_t value) {
    uint8_t* pa;
    Errno err = tables_[active_idx_].translate(va, &pa, WRITE);
    if (err != Errno::SUCCESS) {
        return err;
    }

    *pa = static_cast<uint8_t>(value);
    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::writeI16(uint32_t va, int16_t value) {
    uint8_t* pa[sizeof(int16_t)];

    for (int i = 0; i < sizeof(int16_t); ++i) {
        Errno err = tables_[active_idx_].translate(va + i, &pa[i], WRITE);
        if (err != Errno::SUCCESS) {
            return err;
        }
    }

    for (int i = 0; i < sizeof(int16_t); ++i) {
        *pa[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }

    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::writeI32(uint32_t va, int32_t value) {
    uint8_t* pa[sizeof(int32_t)];

    for (int i = 0; i < sizeof(int32_t); ++i) {
        Errno err = tables_[active_idx_].translate(va + i, &pa[i], WRITE);
        if (err != Errno::SUCCESS) {
            return err;
        }
    }

    for (int i = 0; i < sizeof(int32_t); ++i) {
        *pa[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }

    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::writeI64(uint32_t va, int64_t value) {
    uint8_t* pa[sizeof(int64_t)];

    for (int i = 0; i < sizeof(int64_t); ++i) {
        Errno err = tables_[active_idx_].translate(va + i, &pa[i], WRITE);
        if (err != Errno::SUCCESS) {
            return err;
        }
    }

    for (int i = 0; i < sizeof(int64_t); ++i) {
        *pa[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }

    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::memset(uint32_t va, uint8_t value, size_t count) {
    uint8_t* pa;
    
    for (size_t i = 0; i < count; ++i) {
        Errno err = tables_[active_idx_].translate(va + i, &pa, WRITE);
        if (err != Errno::SUCCESS) {
            return err;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        Errno err = tables_[active_idx_].translate(va + i, &pa, WRITE);
        if (err != Errno::SUCCESS) {
            return err;
        }
        *pa = value;
    }

    return Errno::SUCCESS;
}

Errno MemoryManagementUnit::checkAccess(uint32_t va, AccessType access_type) {
    uint8_t* pa;
    return tables_[active_idx_].translate(va, &pa, access_type);
}

Errno PageTable::mapPage(uint32_t virt_addr, uint32_t phys_addr, uint8_t* real_addr, int prot) {
    uint16_t page_num = virt_addr >> 16;

    if (mapping_.find(page_num) != mapping_.end()) {
        return Errno::EXIST;
    }

    mapping_[page_num] = Mapping{phys_addr, real_addr, prot};
    return Errno::SUCCESS;
}

Errno PageTable::unmapPage(uint32_t virt_addr, uint32_t* phys_addr, int* prot) {
    uint16_t page_num = virt_addr >> 16;

    auto it = mapping_.find(page_num);
    if (it == mapping_.end()) {
        return Errno::BAD_ADDRESS;
    }

    const Mapping& mapping = it->second;
    if (phys_addr) {
        *phys_addr = mapping.phys_addr;
    }
    if (prot) {
        *prot = mapping.prot;
    }

    mapping_.erase(it);
    return Errno::SUCCESS;
}

Errno PageTable::getPage(uint32_t virt_addr, uint32_t* phys_addr, int* prot) {
    uint16_t page_num = virt_addr >> 16;

    auto it = mapping_.find(page_num);
    if (it == mapping_.end()) {
        return Errno::BAD_ADDRESS;
    }

    const Mapping& mapping = it->second;
    if (phys_addr) {
        *phys_addr = mapping.phys_addr;
    }
    if (prot) {
        *prot = mapping.prot;
    }

    return Errno::SUCCESS;
}

Errno PageTable::translate(uint32_t va, uint8_t** pa, AccessType access_type) {
    uint16_t page_num = va >> 16;
    uint16_t offset = va & 0xFFFF;

    auto it = mapping_.find(page_num);
    if (it == mapping_.end()) {
        return Errno::BAD_ADDRESS;
    }

    const Mapping& mapping = it->second;
    if (access_type == READ && !(mapping.prot & READABLE)) {
        return Errno::PERMISSION_DENIED;
    }
    if (access_type == WRITE && !(mapping.prot & WRITABLE)) {
        return Errno::PERMISSION_DENIED;
    }

    *pa = mapping.real_addr + offset;
    return Errno::SUCCESS;
}

runtime::Continuation
MemoryManagementUnit::fault(runtime::Instance& instance, uint32_t addr, bool is_write) {
    runtime::Context& ctxt = instance.getActiveContext();
    ctxt.pushI32(addr);
    ctxt.pushI32(is_write);
    return page_fault_handler_.get();
}
