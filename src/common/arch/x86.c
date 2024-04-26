#include "arch.h"
#include <stdio.h>
#include <memory.h>

lm_arch_t
get_architecture()
{
	return LM_ARCH_X86;
}

lm_size_t
generate_hook_payload(lm_address_t from, lm_address_t to, lm_size_t bits, lm_byte_t **payload_out)
{
	char code[255];
	lm_size_t size;

	if (bits == 64) {
		/*
		 * NOTE: There is an 8-byte long nop after the jmp, because
		 *       that's where we will put the jump address. This avoids
		 *       modifying registers.
		 */
		snprintf(code, sizeof(code), "jmp [rip]; nop; nop; nop; nop; nop; nop; nop; nop");
	} else {
		snprintf(code, sizeof(code), "jmp 0x%x", (unsigned int)to);
	}
	
	size = LM_AssembleEx(code, get_architecture(), bits, from, payload_out);

	if (size > 0 && bits == 64) {
		/* Patch the jump address into the payload */
		lm_byte_t *payload = *payload_out;

		*(uint64_t *)(&payload[size - sizeof(uint64_t)]) = (uint64_t)to;
	}

	return size;
}

lm_size_t
generate_no_ops(lm_byte_t *buf, lm_size_t size)
{
	memset(buf, 0x90, size);
	return size;
}
