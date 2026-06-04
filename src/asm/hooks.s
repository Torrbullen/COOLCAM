# Macro for creating raw patches.
.macro PATCH addr, name, instr:vararg
	.section .hook.\name 
	\instr
.endm 

# --- Entrypoints ---

# Hooked to controller input.
PATCH 0x97ca18, pad_redir_patch, bl pad_redirect

# --- Patches ---
# Skip intro, I use it to boot the game faster on RPCS3 mainly.
PATCH 0x1113c, introskip, b 0x4b8
PATCH 0x9027a0, modeLobbyInit_skip, blr
PATCH 0x984bb8, patch_rpcs3_boot_fail, blr

# unbinds camera from ratchet.
PATCH 0xf0f78, sfdcga, ba camlock1
PATCH 0x10ce14, gfdwjy, ba camlock2
#PATCH 0x19624c, noreticle, cmpwi r3,0xfff
PATCH 0x1e325c, fcghjfa, ba quickselect_hide 
