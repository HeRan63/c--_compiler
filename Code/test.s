.data
_prompt: .asciiz ""
_ret: .asciiz "\n"
.globl main
.text
read:
	li $v0, 4
	la $a0, _prompt
	syscall
	li $v0, 5
	syscall
	jr $ra

write:
	li $v0, 1
	syscall
	li $v0, 4
	la $a0, _ret
	syscall
	move $v0, $0
	jr $ra

main:
	addi $sp, $sp, -8
	sw $fp, 0($sp)
	sw $ra, 4($sp)
	move $fp, $sp
	addi $sp, $sp, -16
	li $t0, 2
	move $a0, $t0
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	jal write
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	li $t0, 0
	lw $t1, -4($fp)
	move $t1, $t0
	sw $t1, -4($fp)
	li $t0, 0
	move $a0, $t0
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	jal write
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	li $t0, 0
	lw $t1, -8($fp)
	move $t1, $t0
	sw $t1, -8($fp)
	li $t0, 1
	move $a0, $t0
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	jal write
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	li $t0, 0
	lw $t1, -12($fp)
	move $t1, $t0
	sw $t1, -12($fp)
	li $t0, 7
	move $a0, $t0
	addi $sp, $sp, -4
	sw $ra, 0($sp)
	jal write
	lw $ra, 0($sp)
	addi $sp, $sp, 4
	li $t0, 0
	lw $t1, -16($fp)
	move $t1, $t0
	sw $t1, -16($fp)
	lw $ra, 4($fp)
	addi $sp, $fp, 8
	li $t0, 0
	lw $fp, 0($fp)
	move $v0, $t0
	jr $ra
