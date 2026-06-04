# Start of globals. enablemod should be at the start of the struct ALWAYS.
# Or else this will fail to work
.equ enablemod, 0xD9F000

.global camlock1
camlock1:
    lis     r9, enablemod@ha
    lwz     r10, enablemod@l(r9)
    cmpwi   r10, 1
    bne     .exec
    blr
.exec:
	stdu	r1, -0x150(r1)
    ba      0xf0f7c

.global camlock2
camlock2:
    lis     r9, enablemod@ha
    lwz     r10, enablemod@l(r9)
    cmpwi   r10, 1
    bne     .exec2
    blr
.exec2:
	stdu	r1, -0x90(r1)
    ba      0x10ce18

.global quickselect_hide
quickselect_hide:
    lis     r9, enablemod@ha
    lwz     r10, enablemod@l(r9)
    cmpwi   r10, 1
    bne     .exec3
    blr
.exec3:
	stdu	r1, -0xa0(r1)
    ba      0x1e3260
