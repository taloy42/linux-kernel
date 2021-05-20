#include "os.h"
#include <stdio.h>

void* ppn_to_virt(uint32_t ppn){
	return phys_to_virt(ppn<<12);
}

/*
 * pt  : **physical** page number of page table root
 * vpn : virtual page number the caller wishes to (un)map
 * ppn : if NO_MAPPING, then we unmap. else, we map vpn to ppn
 *
 * maps vpn to ppn
 */
void page_table_update(uint32_t pt, uint32_t vpn, uint32_t ppn){
	uint32_t *ptbr =  (uint32_t*)phys_to_virt(pt<<12);
	
	uint32_t L1 = vpn>>10;
	uint32_t L2 = vpn & 0x3ff;
	
	uint32_t pte_1 = *(ptbr+L1);
	if ((pte_1&0x1)==0){
		if (ppn==NO_MAPPING){
			return;
		}
		else {
			pte_1 = (alloc_page_frame()<<12) + 1;
			*(ptbr+L1)=pte_1;
		}
	}
	
	uint32_t *ref_1 = phys_to_virt(pte_1-1);
	*(ref_1+L2)=(ppn<<12)+(ppn!=NO_MAPPING);
}

/*
 * pt : **physical** page number of page table root
 * virtual page number to search
 *
 * returns the ppn which vpn is mapped to
 */
uint32_t page_table_query(uint32_t pt, uint32_t vpn){
	uint32_t *ptbr =  (uint32_t*)phys_to_virt(pt<<12);
	
	uint32_t L1 = vpn>>10;
	uint32_t L2 = vpn & 0x3ff;
	
	uint32_t pte_1 = *(ptbr+L1);
	if ((pte_1&0x1)==0){
		return NO_MAPPING;
	}
	uint32_t *ref_1 = phys_to_virt(pte_1-1);

	uint32_t pte_2 = *(ref_1+L2);
    if (pte_2 & 0x1){
        return pte_2>>12;
    }
	return NO_MAPPING;
}
