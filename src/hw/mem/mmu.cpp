#include "hw/mem/mmu.hpp"
#include "runtime/instance.hpp"

namespace hw::mem {

Errno MemoryManagementUnit::createTable(uint32_t& ptable_idx_out) {
    if (free_list_.empty()) {
        ptable_idx_out = static_cast<uint32_t>(tables_.size());
        tables_.emplace_back();
    } else {
        ptable_idx_out = free_list_.top();
        free_list_.pop();
    }

    return Errno::success;
}

Errno MemoryManagementUnit::destroyTable(uint32_t ptable_idx) {
    if (ptable_idx >= tables_.size()) {
        return Errno::invalid;
    }

    tables_[ptable_idx].clear();
    free_list_.push(ptable_idx);
    return Errno::success;
}

Errno MemoryManagementUnit::loadTable(uint32_t ptable_idx) {
    if (ptable_idx >= tables_.size())
        return Errno::invalid;

    active_idx_ = ptable_idx;
    return Errno::success;
}

Errno MemoryManagementUnit::mapPage(uint32_t ptable_idx, uint32_t virt_addr,
                                    uint32_t offset, int32_t prot) {
    if (ptable_idx >= tables_.size() || (virt_addr & WPAGE_MASK) ||
        (offset & WPAGE_MASK))
        return Errno::invalid;

    if (!memory_.contains(offset, WPAGE_SIZE))
        return Errno::BAD_ADDRESS;

    return tables_[ptable_idx].mapPage(virt_addr, offset, prot);
}

Errno MemoryManagementUnit::unmapPage(uint32_t ptable_idx, uint32_t virt_addr,
                                      uint32_t* offset_out, int32_t* prot_out) {
    if (ptable_idx >= tables_.size())
        return Errno::invalid;

    return tables_[ptable_idx].unmapPage(virt_addr, offset_out, prot_out);
}

Errno MemoryManagementUnit::getPage(uint32_t ptable_idx, uint32_t virt_addr,
                                    uint32_t* offset_out, int32_t* prot_out) {
    if (ptable_idx >= tables_.size())
        return Errno::invalid;

    return tables_[ptable_idx].getPage(virt_addr, offset_out, prot_out);
}

/*
Errno MemoryManagementUnit::checkAccess(uint32_t va, AccessType access_type) {
    uint8_t* pa;
    return tables_[active_idx_].translate(va, &pa, access_type);
}
*/

Errno PageTable::mapPage(uint32_t virt_addr, uint32_t offset, int32_t prot) {
    uint16_t page_num = virt_addr >> 16;
    mapping_[page_num] = Mapping{offset, prot};
    return Errno::success;
}

Errno PageTable::unmapPage(uint32_t virt_addr, uint32_t* offset,
                           int32_t* prot) {
    uint16_t page_num = virt_addr >> 16;

    auto it = mapping_.find(page_num);
    if (it == mapping_.end())
        return Errno::BAD_ADDRESS;

    const Mapping& mapping = it->second;
    if (offset)
        *offset = mapping.offset;

    if (prot)
        *prot = mapping.prot;

    mapping_.erase(it);
    return Errno::success;
}

Errno PageTable::getPage(uint32_t virt_addr, uint32_t* offset, int32_t* prot) {
    uint16_t page_num = virt_addr >> 16;

    auto it = mapping_.find(page_num);
    if (it == mapping_.end()) {
        return Errno::BAD_ADDRESS;
    }

    const Mapping& mapping = it->second;
    if (offset)
        *offset = mapping.offset;

    if (prot)
        *prot = mapping.prot;

    return Errno::success;
}

bool PageTable::translate(uint32_t virt_addr, uint32_t& offset,
                          AccessType access_type) {
    uint16_t page_num = virt_addr >> WPAGE_WIDTH;
    uint16_t page_offset = virt_addr & WPAGE_MASK;

    auto it = mapping_.find(page_num);
    if (it == mapping_.end())
        return false;

    const Mapping& mapping = it->second;
    if (access_type == READ && !(mapping.prot & READABLE))
        return false;

    if (access_type == WRITE && !(mapping.prot & WRITABLE))
        return false;

    offset = mapping.offset + page_offset;
    return true;
}

runtime::Continuation MemoryManagementUnit::fault(runtime::Instance& instance,
                                                  uint32_t addr,
                                                  bool is_write) {
    runtime::Context& ctxt = instance.getActiveContext();
    ctxt.pushI32(static_cast<int32_t>(addr));
    ctxt.pushI32(is_write);
    return page_fault_handler_.get();
}

} // namespace hw::mem
