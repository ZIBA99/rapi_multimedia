savedcmd_/home/pi/home/gpio_module/gpio_module.mod := printf '%s\n'   gpio_module.o | awk '!x[$$0]++ { print("/home/pi/home/gpio_module/"$$0) }' > /home/pi/home/gpio_module/gpio_module.mod
