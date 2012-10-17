
dwm_dir=${HOME}/.dwm

killall conky; conky -c ${dwm_dir}/conkyrc | while read -r; do 
         
 xsetroot -name "$REPLY"; 

done &

#nitrogen --restore &

parcellite &

#pidgin &

#fcitx &

#xfce4-clipman &

#update-notifier &

#认证代理
#/usr/lib/policykit-1-gnome/polkit-gnome-authentication-agent-1 &

#检测文件的改变
#xdg-user-dirs-gtk-update &

feh --bg-fill /home/lijy/Documents/back.jpg  &
nm-applet &
gnome-sound-applet &
while xsetroot -name "`date "+%F %A %R"`"
do
	sleep 1m
done &


