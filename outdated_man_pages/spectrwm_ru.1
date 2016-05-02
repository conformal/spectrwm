.\" Copyright (c) 2009 Marco Peereboom <marco@peereboom.us>
.\" Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
.\"
.\" Permission to use, copy, modify, and distribute this software for any
.\" purpose with or without fee is hereby granted, provided that the above
.\" copyright notice and this permission notice appear in all copies.
.\"
.\" THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
.\" WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
.\" MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
.\" ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
.\" WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
.\" ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
.\" OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
.\"
.Dd $Mdocdate$
.Dt SPECTRWM 1
.Os
.Sh НАЗВАНИЕ
.Nm spectrwm
.Nd Оконный менеджер для X11
.Sh ИСПОЛЬЗОВАНИЕ
.Nm spectrwm
.Sh ОПИСАНИЕ
.Nm
это минималистичный менеджер окон, ставящий своей целью не мешать вам и не
занимать ценное пространство экрана. Его настройки по-умолчанию разумны и,
кроме того, он не требует знания языков программирования для работы с
конфигурационным файлом. Он написан хакерами для хакеров и старается быть
легким, компактным и быстрым.
.Pp
Когда
.Nm
запускается, он читает настройки из своего конфигурационного файла,
.Pa spectrwm.conf .
Смотрите секцию
.Sx КОНФИГУРАЦИОННЫЕ ФАЙЛЫ
ниже.
.Pp
На этой странице используются следующие обозначения:
.Pp
.Bl -tag -width Ds -offset indent -compact
.It Cm M
Мета-клавиша
.It Cm S
Shift
.It Aq Cm Name
Имя клавиши
.It Cm M1
Кнопка мыши 1
.It Cm M3
Кнопка мыши 3
.El
.Pp
.Nm
должен быть понятным и очевидным.
Большинство действий выполняется комбинациями клавиш.
Смотрите секцию
.Sx ПРИВЯЗКИ
ниже, чтобы узнать о стандартных настройках.
.Sh КОНФИГУРАЦИОННЫЕ ФАЙЛЫ
.Nm
пытается прочитать файл в домашнем каталоге,
.Pa ~/.spectrwm.conf .
В случае, если он недоступен,
происходит обращение к глобальному файлу настроек,
.Pa /etc/spectrwm.conf .
.Pp
Формат файла следующий: \*(Ltключ\*(Gt = \*(Ltзначение\*(Gt.
Например:
.Pp
.Dl color_focus = red
.Pp
Однозначное включение и выключение задается значениями 1 и 0.
.Pp
Поддерживаются следующие ключевые слова:
.Pp
.Bl -tag -width "title_class_enabledXXX" -offset indent -compact
.It Cm color_focus
Цвет рамки окна в фокусе.
.It Cm color_unfocus
Цвет рамки окон не в фокусе.
.It Cm bar_enabled
Включение статусной строки.
.It Cm bar_border Ns Bq Ar x
Цвет рамки статусной строки
.Ar x .
.It Cm bar_color Ns Bq Ar x
Цвет статусной строки
.Ar x .
.It Cm bar_font_color Ns Bq Ar x
Цвет шрифта статусной строки
.Ar x .
.It Cm bar_font
Тип шрифта статусной строки.
.It Cm bar_action
Внешний файл скрипта для статусной строки, выводящий туда информацию,
например, уровень заряда батарей.
.It Cm stack_enabled
Включить отображение способа укладки окон в статусной строке.
.It Cm clock_enabled
Включить часы в статусной строке.
Можно отключить, установив 0, и Вы сможете использовать
собственные часы из внешнего скрипта.
.It Cm dialog_ratio
Ряд приложений имеет слишком маленькие диалоговые окна.
Это значение - доля размера экрана, к которой они будут приведены.
Например, значение 0.6 будет соответствовать 60% от реального размера экрана.
.It Cm region
Выделяет область экрана на Ваше усмотрение, уничтожает все перекрытые области
экрана, определенные автоматически.
Формат: screen[<idx>]:WIDTHxHEIGHT+X+Y,
например\& screen[1]:1280x800+0+0.
.It Cm term_width
Установить минимальную допустимую ширину эмулятора терминала.
Если это значение больше 0,
.Nm
попытается отмасштабировать шрифты в терминале, чтобы ширина
была больше этого значения
.
Поодерживается только
.Xr xterm 1
.
Также
.Xr xterm 1
не может быть с setuid или setgid, хотя это так на многих системах.
Возможно необходимо задать program[term] (Смотрите секцию
.Sx ПРОГРАММЫ
) чтобы использовалась другая копия
.Xr xterm 1
без заданного бита setgid.
.It Cm title_class_enabled
Отображать класс окна в статусной строке.
Обычно выключено
.It Cm title_name_enabled
Отображать заголовок окна в статусной строке.
Обычно выключено
.It Cm modkey
Назначить Мета-клавишу, клавишу-модификатор.
Mod1 соответствует клавише ALT, а Mod4 соответствует клавише WIN на PC.
.It Cm program Ns Bq Ar p
Добавить пользовательскую программу для назначения привязки
.Ar p .
Смотрите секцию
.Sx ПРОГРАММЫ
ниже.
.It Cm bind Ns Bq Ar x
Назначить привязку на действие
.Ar x .
Смотрите секцию
.Sx ПРИВЯЗКИ
ниже.
.It Cm quirk Ns Bq Ar c:n
Добавить костыль для окон с классом
.Ar c
и именем
.Ar n .
Смотрите секцию
.Sx КОСТЫЛИ
ниже.
.El
.Pp
Цвета задаются с помощью
.Xr XQueryColor 3
А шрифты задаются с использованием
.Xr XQueryFont 3
.
.Sh ПРОГРАММЫ
.Nm
позволяет Вам добавлять Ваши собственные действия для запуска
программ и делать к ним привязки как ко всем остальным действиям
Смотрите секцию
.Sx ПРИВЯЗКИ
ниже.
.Pp
Стандартные программы:
.Pp
.Bl -tag -width "screenshot_wind" -offset indent -compact
.It Cm term
xterm
.It Cm screenshot_all
screenshot.sh full
.It Cm screenshot_wind
screenshot.sh window
.It Cm lock
xlock
.It Cm initscr
initscreen.sh
.It Cm menu
dmenu_run \-fn $bar_font \-nb $bar_color \-nf $bar_font_color \-sb $bar_border \-sf $bar_color
.El
.Pp
Ваши собственные программы задаются следующим образом:
.Pp
.Dl program[<name>] = <progpath> [<arg> [... <arg>]]
.Pp
.Aq name
это любой идентификатор, не мешающийся с уже существующими,
.Aq progpath
это собственно путь к программе,
.Aq arg
это список передаваемых аргументов или оставьте пустым.
.Pp
Следующие переменные можно получать из
.Nm
(Смотрите секцию
.Sx КОНФИГУРАЦИОННЫЕ ФАЙЛЫ
выше),
и их можно использовать как
.Aq arg
(в момент запуска программы будет выполнена подстановка значений):
.Pp
.Bl -tag -width "$bar_font_color" -offset indent -compact
.It Cm $bar_border
.It Cm $bar_color
.It Cm $bar_font
.It Cm $bar_font_color
.It Cm $color_focus
.It Cm $color_unfocus
.El
.Pp
Например:
.Bd -literal -offset indent
program[ff] = /usr/local/bin/firefox http://spectrwm.org/
bind[ff] = Mod+f # Значит Mod+F запускает firefox
.Ed
.Pp
Чтобы отменить назначение:
.Bd -literal -offset indent
bind[] = Mod+f
program[ff] =
.Ed
.Pp
.Sh ПРИВЯЗКИ
.Nm
предоставляет доступ к действиям с помощью клавиатурных комбинаций.
.Pp
Установленные привязки для мыши:
.Pp
.Bl -tag -width "M-j, M-<TAB>XXX" -offset indent -compact
.It Cm M1
Сфокусироваться на окне
.It Cm M-M1
Переместить окно
.It Cm M-M3
Изменить размер окна
.It Cm M-S-M3
Изменить размер окна, удерживая его в центре
.El
.Pp
Стандартные клавиатурные привязки:
.Pp
.Bl -tag -width "M-j, M-<TAB>XXX" -offset indent -compact
.It Cm M-S- Ns Aq Cm Return
term
.It Cm M-p
menu
.It Cm M-S-q
quit
.It Cm M-q
restart
.Nm
.It Cm M- Ns Aq Cm Space
cycle_layout
.It Cm M-S- Ns Aq Cm Space
reset_layout
.It Cm M-h
master_shrink
.It Cm M-l
master_grow
.It Cm M-,
master_add
.It Cm M-.
master_del
.It Cm M-S-,
stack_inc
.It Cm M-S-.
stack_del
.It Cm M- Ns Aq Cm Return
swap_main
.It Xo
.Cm M-j ,
.Cm M- Ns Aq Cm TAB
.Xc
focus_next
.It Xo
.Cm M-k ,
.Cm M-S- Ns Aq Cm TAB
.Xc
focus_prev
.It Cm M-m
focus_main
.It Cm M-S-j
swap_next
.It Cm M-S-k
swap_prev
.It Cm M-b
bar_toggle
.It Cm M-x
wind_del
.It Cm M-S-x
wind_kill
.It Cm M- Ns Aq Ar n
.Ns ws_ Ns Ar n
.It Cm M-S- Ns Aq Ar n
.Ns mvws_ Ns Ar n
.It Cm M- Ns Aq Cm Right
ws_next
.It Cm M- Ns Aq Cm Left
ws_prev
.It Cm M-S- Ns Aq Cm Right
screen_next
.It Cm M-S- Ns Aq Cm Left
screen_prev
.It Cm M-s
screenshot_all
.It Cm M-S-s
screenshot_wind
.It Cm M-S-v
version
.It Cm M-t
float_toggle
.It Cm M-S Aq Cm Delete
lock
.It Cm M-S-i
initscr
.El
.Pp
Описания действий перечислены ниже:
.Pp
.Bl -tag -width "M-j, M-<TAB>XXX" -offset indent -compact
.It Cm term
Запустить эмулятор терминала
(Смотрите секцию
.Sx ПРОГРАММЫ
выше)
.It Cm menu
Меню
(Смотрите секцию
.Sx ПРОГРАММЫ
выше)
.It Cm quit
Выйти
.Nm
.It Cm restart
Перезапустить
.Nm
.It Cm cycle_layout
Менять укладку окон
.It Cm reset_layout
Стандартная укладка
.It Cm master_shrink
Сжать область главного окна
.It Cm master_grow
Расширить область главного окна
.It Cm master_add
Добавить окна в главную область
.It Cm master_del
Убрать окна из главной области
.It Cm stack_inc
Увеличить число столбцов или рядов в текущей укладке
.It Cm stack_del
Уменьшить число столбцов или рядов в текущей укладке
.It Cm swap_main
Отправить текущее окно в главную область, сделать главным
.It Cm focus_next
Фокусироваться на следующем окне
.It Cm focus_prev
Фокусироваться на предыдущем окне
.It Cm focus_main
Фокусироваться на главном окне
.It Cm swap_next
Поменять со следующим окном
.It Cm swap_prev
Поменять со предыдущим окном
.It Cm bar_toggle
Выключить статусную строку на всех рабочих столах
.It Cm wind_del
Закрыть фокусированное окно
.It Cm wind_kill
Грохнуть фокусированное окно
.It Cm ws_ Ns Ar n
Переключиться на рабочий стол
.Ar n ,
где
.Ar n
от 1 до 10
.It Cm mvws_ Ns Ar n
Переместить фокусированное окно в рабочий стол
.Ar n ,
где
.Ar n
от 1 до 10
.It Cm ws_next
Перейти к следующему не пустому рабочему столу
.It Cm ws_prev
Перейти к следующему не пустому рабочему столу
.It Cm screen_next
Переместить указатель в следующую область
.It Cm screen_prev
Переместить указатель в следующую область
.It Cm screenshot_all
Сделать снимок всего экрана (если возможно)
(Смотрите секцию
.Sx ПРОГРАММЫ
выше)
.It Cm screenshot_wind
Сделать снимок окна (если возможно)
(Смотрите секцию
.Sx ПРОГРАММЫ
выше)
.It Cm version
Показать версию в статусной строке
.It Cm float_toggle
Переключить окно в фокусе в плавающий режим, float
.It Cm lock
Заблокировать экран
(Смотрите секцию
.Sx ПРОГРАММЫ
выше)
.It Cm initscr
Инициализировать экран еще раз
(Смотрите секцию
.Sx ПРОГРАММЫ
выше)
.El
.Pp
Собственные привязки назначаются следующим образом:
.Pp
.Dl bind[<action>] = <keys>
.Pp
.Aq action
это действие из списка программ
.Aq keys
это не более одной клавиши-модификатора
(MOD, Mod1, Shift, и.т.п.) и обычные клавиши
(b, space, и.т.п.), разделенные "+".
Например:
.Bd -literal -offset indent
bind[reset] = Mod4+q # назначить WIN + q на действие reset
bind[] = Mod1+q # снять все действия с Alt + q
.Ed
.Pp
На одно действие можно назначить несколько комбинаций.
.Sh КОСТЫЛИ
.Nm
позволяет настроить костыли, нужные для специальной работы spectrwm
с рядом приложений, который вы определяете сами. То есть, Вы можете
принудительно установить способ тайлинга для какого-нибудь приложения
.Pp
Список стандартных костылей:
.Pp
.Bl -tag -width "OpenOffice.org N.M:VCLSalFrame<TAB>XXX" -offset indent -compact
.It Firefox\-bin:firefox\-bin
TRANSSZ
.It Firefox:Dialog
FLOAT
.It Gimp:gimp
FLOAT + ANYWHERE
.It MPlayer:xv
FLOAT + FULLSCREEN
.It OpenOffice.org 2.4:VCLSalFrame
FLOAT
.It OpenOffice.org 3.1:VCLSalFrame
FLOAT
.It pcb:pcb
FLOAT
.It xine:Xine Window
FLOAT + ANYWHERE
.It xine:xine Panel
FLOAT + ANYWHERE
.It xine:xine Video Fullscreen Window
FULLSCREEN + FLOAT
.It Xitk:Xitk Combo
FLOAT + ANYWHERE
.It Xitk:Xine Window
FLOAT + ANYWHERE
.It XTerm:xterm
XTERM_FONTADJ
.El
.Pp
Описание:
.Pp
.Bl -tag -width "XTERM_FONTADJ<TAB>XXX" -offset indent -compact
.It FLOAT
Такое окно не нужно тайлить вообще, разрешить ему float
.It TRANSSZ
Тразиентое окно
(Смотрите секцию
.Sx КОНФИГУРАЦИОННЫЕ ФАЙЛЫ) .
.It ANYWHERE
Позволить окну самостоятельно выбрать местоположение
.It XTERM_FONTADJ
Изменять шрифты xterm при изменении размеров окна
.It FULLSCREEN
Позволить окну запускаться в полноэкранном режиме
.El
.Pp
Назначать костыли можно следующим образом:
.Pp
.Dl quirk[<class>:<name>] = <quirk> [ + <quirk> ... ]
.Pp
.Aq class
и
.Aq name
определяют к какому окну будет применяться костыль, а
.Aq quirk
один из вышеперечисленных способов.
Например:
.Bd -literal -offset indent
quirk[MPlayer:xv] = FLOAT + FULLSCREEN # mplayer настроен
quirk[pcb:pcb] = NONE  # убрать существующий костыль
.Ed
.Pp
Вы можете узнать
.Aq class
и
.Aq name
запустив xprop и нажав в интересующее окно.
Вот как будет выглядеть вывод для Firefox:
.Bd -literal -offset indent
$ xprop | grep WM_CLASS
WM_CLASS(STRING) = "Navigator", "Firefox"
.Ed
.Pp
Обратите внимание, класс и имя меняются местами,
правильный костыль будет выглядеть так:
.Bd -literal -offset indent
quirk[Firefox:Navigator] = FLOAT
.Ed
.Sh ФАЙЛЫ
.Bl -tag -width "/etc/spectrwm.confXXX" -compact
.It Pa ~/.spectrwm.conf
.Nm
Личные настройки пользователя.
.It Pa /etc/spectrwm.conf
.Nm
Глобавльные настройки.
.El
.Sh ИСТОРИЯ
.Nm
идейно основан на dwm и xmonad
.Sh АВТОРЫ
.An -nosplit
.Pp
.Nm
написан:
.An Marco Peereboom Aq marco@peereboom.us ,
.An Ryan Thomas McBride Aq mcbride@countersiege.com
and
.An Darrin Chandler Aq dwchandler@stilyagin.com .
.Sh БАГИ
При вызове меню с помощью
.Cm M-p ,
необходима корректная работа dmenu.
