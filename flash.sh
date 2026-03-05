make -C gcc && openocd -f board/ti_msp432_launchpad.cfg -c "program gcc/main.out verify reset exit"
