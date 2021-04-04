#!/bin/bash
# screen -XS [ID] quit
touch /home/pi/Desktop/GUI_Worker.sh 
echo "#!/bin/bash
screen -S GUI_Worker" >> /home/pi/Desktop/GUI_Worker.sh
chmod +x /home/pi/Desktop/GUI_Worker.sh