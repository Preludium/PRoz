#!/bin/bash

#KDIALOG=`which kdialog`
KDIALOG=kdialog
#COMMON_OPTIONS="--yes-label Tak --no-label Nie --cancel-label Anuluj --continue-label Dalej"
# dialog mi nie działa w $(dialog), bo wyświetla kody sterujące dla terminala.
#nie chce mi się myśleć, jak to zrobić uniwersalnie
#DIALOG_MENU_OPT="80 100 40"
#DEFAULT_OPT=default-item
DEFAULT_OPT=default
TEST=0
LAB=`hostname | cut -f 1-2 -d-`

if [ -e ${HOME}/mpi/.repo ];then
    REPO=`cat ${HOME}/mpi/.repo`
else
    REPO=http://www.cs.put.poznan.pl/adanilecki/pr/mpi
fi
#TEMATY="bank broadcast global-state mattern photo-scale pi passwd term-detect token-detect"
TEMATY=$(cat ${HOME}/mpi/.tematy | cut -f 1 -d:)
declare -A TEMATY_ASSOC

OLD="${IFS}"
IFS=+
for i in $(tr "\n" + < ${HOME}/mpi/.tematy); do
    SHORT_NAME=`echo $i | cut -f 1 -d:`
    LONG_NAME=`echo $i | cut -f 2 -d:`
    #echo $i=$SHORT_NAME:$LONG_NAME
    TEMATY_NAZWY=$TEMATY_NAZWY"+"${LONG_NAME}
    TEMATY_ASSOC[${LONG_NAME}]=${SHORT_NAME}
done	
#IFS=' 	'
#echo -n "${IFS}" | hexdump
IFS="${OLD}"
#echo -n"${IFS}" | hexdump
#exit
unset O
unset LONG_NAME
unset SHORT_NAME


if [ -z "$KDIALOG" ]; then
    echo "Nie mogę odnaleźć programu kdialog. Wychodzę"
    exit
fi

function get_lab() {
    if [ $TEST -eq 1 ]; then return 0; fi

    echo Pobieram pliki dla laboratorium "$1"
    wget ${REPO}/$1.tgz
    if [ -e $1.tgz ]; then
        mv $1.tgz ${HOME}/mpi
	pushd ${HOME}/mpi/ 
        #mkdir $1 2>/dev/null
        #cd $1
        rm -rf $1
        #mv ../$1.tgz .
	tar xvfz $1.tgz
        rm $1.tgz
	popd
        return 0
    fi
    return 1
}

function init_env() {
    if [ $TEST -eq 1 ]; then return 0; fi
    cp -r ${HOME}/mpi ${HOME}/mpi-backup-`date +%F`
    rm -rf ${HOME}/mpi 2>/dev/null
    mkdir ${HOME}/mpi 2>/dev/null
    echo "${REPO}" > ${HOME}/mpi/.repo 2>/dev/null
    wget ${REPO}/tematy 2>/dev/null
    mv tematy ${HOME}/mpi/.tematy 2>/dev/null
    wget ${REPO}/bashrc-${LAB} 2>/dev/null
    cat bashrc-${LAB} >> ${HOME}/.bashrc
    get_lab first 
    echo "Zamknij i otwórz konsolę tekstową lub wydaj polecenie \". ~/.bashrc\""
}

function init_ssh() {
    if [ $TEST -eq 1 ]; then return; fi
    rm -f ${HOME}/.ssh/id_rsa 2>/dev/null
    rm -f ${HOME}/.ssh/authorized_keys 2>/dev/null
    ssh-keygen -f ${HOME}/.ssh/id_rsa -q -N ""
    cp ${HOME}/.ssh/id_rsa.pub ${HOME}/.ssh/authorized_keys
}

function update_repo() {
    if [ $TEST -eq 1 ]; then return 0; fi
    wget ${REPO}/tematy 

    if [ ! -e ${HOME}/mpi ]; then 
        mkdir ${HOME}/mpi
    fi

    if [ -e tematy ]; then 
	mv tematy ${HOME}/mpi/.tematy
    else 
	echo Błąd w aktualizacji tematów
        return 1
    fi
    wget ${REPO}/readme.txt 
    if [ -e readme.txt ]; then 
	mv readme.txt ${HOME}/mpi/
    else 
	echo Błąd w aktualizacji pliku readme.txt
        return 1
    fi

    wget ${REPO}/updater 2>/dev/null
    bash updater
    rm updater

    return 0
}

mkdir ${HOME}/mpi 2>/dev/null

# :: oznacza optional argument
OPCJE=`getopt -o "i:g:htlku" -l "help,get-lab:,list-lab,init:,init-ssh,test,init-keys,update" -- $@`

eval set -- "$OPCJE"

OPT=0
while test "$1"; do
        case $1 in
	    -h|--help)
		OPT=1
                echo
		echo Następujące opcje są dostępne:
                cat <<END

-g <temat>, --get-lab <temat> : Pobranie plików potrzebnych do laboratorium 
                                na dany temat
-l, --list-lab : wypisanie listy tematów laboratoriów
-h, --help: pomoc, którą czytasz
-i, --init : inicjalizacja środowiska i ściągnięcie pierwszego laboratorium
--init-ssh : inicjalizacja kluczy publicznych i prywatnych ssh
-u, --update : aktualizacja tematów
-t, --test : funkcje nie będą naprawdę wykonywane, pokazywane będą tylko 
            okienka (w trybie graficznym). Tak samo będzie, jeżeli -t będzie
            pierwszą opcją w trybie wsadowym. 

Wykorzystywane są następujące pliki konfiguracyjne: 
    ${HOME}/mpi/.tematy  : przechowuje listę tematów 
    ${HOME}/mpi/.repo    : przechowuje adres repozytorium, w którym są 
                           szkielety programów oraz lista aktualnych tematów 
                           laboratoriów

END
		shift;;
	    -g|--get-lab) 
                TMP=0
		OPT=1
                OLD=$IFS
                echo "${TEMATY} IFS=:$IFS:"
                for i in ${TEMATY};do
                    if [ "$i" = "$2" ]; then
                        TMP=1
                        get_lab $i
                        break;
                    fi
                done
                if [ $TMP -eq 0 ]; then
                    echo Brak tematu o nazwie $2
                fi
                unset TMP
		shift 2;;
	    -i|--init) echo Inicjalizuję "$2"
		OPT=1
                init_env
		shift 2;;
            -u|--update)
                OPT=1;
                update_repo
                shift;;
            --init-ssh)
		OPT=1
                    init_ssh
                shift;;
            -k,--init-keys)
		OPT=1
                    if [ "$2" = "lab-os" ]; then
                        for i in `seq 1 32`; do
			    ssh-keyscan -t rsa lab-os-$i >> ~/.ssh/known_hosts
                        done
                    elif [ "$2" = "lab-net" ]; then
                        for i in `seq 1 32`; do
			    ssh-keyscan -t rsa lab-net-$i >> ~/.ssh/known_hosts
                        done
                    elif [ "$2" = "lab-sec" ]; then
                        for i in `seq 1 32`; do
			    ssh-keyscan -t rsa lab-sec-$i >> ~/.ssh/known_hosts
                        done
                    fi 
                    ssh-keyscan -t rsa $2 >> ~/.ssh/known_hosts
                shift;;
            -l|--list-lab)
		OPT=1
                echo Lista dostępnych laboratoriów do pobrania:
                echo ${TEMATY}
                shift;;
            -t|--test)
                echo Funkcje w trybie graficznym nie będą wykonywane;
                echo Zostaną tylko pokazane okienka.
                TEST=1
                shift;;
	    --) shift; break;;
	    *) echo nieznana opcja; shift;;
        esac
done

if [ $OPT -eq 1 ]; then
    exit 0
fi

while true; do
    OPT=$($KDIALOG --title "Laboratorium Przetwarzanie Rozproszone, MPI" \
	    --$DEFAULT_OPT lab \
	    --menu "Wybierz pozycję z pól podanych poniżej. 

    Program \"mpi\" został napisany na potrzeby laboratorium z PR Politechniki Poznańskiej 
    i nie stanowi części standardowej instalacji linuksa.        " \
                $DIALOG_MENU_OPT \
		kompilacja "Pomoc na temat kompilowania i uruchamiania programów" \
		init "Inicjalizacja środowiska" \
		update "Aktualizacja tematów laboratoriów i konfiguracji" \
		ssh "Konfiguracja ssh na potrzeby laboratorium" \
		lab "Pobieranie plików na potrzeby danego laboratorium"  \
		pomoc "Pomoc na temat skryptu mpi" \
		koniec "Wyjście" )
    if [ ! $? -eq 0 ]
        then exit 0
    fi

    case $OPT in
	update) 
            update_repo
            if [ $? -eq 0 ];then
	    $KDIALOG --title "Ok!" --msgbox "Ok! Aktualizacja się powiodła" 
            else
	    $KDIALOG --title "Błąd!" --error "Błąd przy aktualizacji" 
            fi;;
	kompilacja) 
	    $KDIALOG --title "Kompilowanie i uruchomianie programów w MPI" \
		--msgbox "Kompilacja programów w MPI wykonywana jest przy pomocy programu mpicc, mpic++ albo mpiCC. Jeżeli program jest napisany w C, i nazywa się first.c, to jego kompilacja do pliku wynikowego o nazwie first.exe wygląda następująco:

	mpicc first.c -o first.exe

    Uruchomienie programu first.exe następuje w najprostszym przypadku tak:

	mpirun -np 6 first.exe

    W przykładzie powyżej uruchomionych zostanie sześć procesów, wszystkie wykonujące kod first.exe i wszystkie uruchomione na lokalnym węźle. W celu uruchomienia procesów na różnych węzłach, należy przygotować plik konfiguracyjny zawierający nazwy węzłów albo ich numery IP, po jednym w każdej linijce, a następnie podać go jako argument dla programu mpirun:

	mpirun -hostfile PLIK_KONFIGURACYJNY -np 6 first.exe

    Nazwy hostów można też podać \"z palca\" w następujący sposób:

	mpirun -H lab-os-1,lab-os-2 -np 6 first.exe

    W przypadku powyżej uruchomionych będzie 6 procesów, z tego trzy na węźle lab-os-1,a dalsze trzy na węźle lab-os-2.


    Czasami zdarza się, że powyższe polecenia nie zadziałają, a mpirun będzie domagał się podania pliku default-hostfile. W takim wypadku uruchamiamy programy w następujący sposób:

        mpirun -default-hostfile none -np 4 first.exe

    ";;
	pomoc) $KDIALOG --title "Pomoc na temat skryptu mpi" --msgbox \
"Skrypt mpi został napisany na potrzeby laboratorium z PR, w celu ułatwienia przygotowania środowiska do pracy z pakietem MPI oraz pobierania plików szkieletowych potrzebnych dla laboratorium.

Posiada następującą funkcjonalność:

• \"Inicjalizacja środowiska\" Przygotowuje środowisko na potrzeby laboratorium. Wystarczy wywołać jednokrotnie; każdorazowe uruchomienie inicjalizuje środowisko ponownie, niszcząc poprzednie zmiany. także konfiguruje ssh.
• \"Pomoc na temat kompilowania i uruchamiania programów\": Wyświetla okienko omawiające metody kompilowania programów za pomocą mpicc oraz ich uruchamianie przy pomocy mpirun.
• \"Konfiguracja ssh na potrzeby laboratorium\" Konfiguruje ssh dla konta studenta, tak, by nie było wymagane podawanie hasła przy każdorazowym łączeniu się z innymi węzłami laboratorium.
• \"Pobieranie plików na potrzeby danego laboratorium\" Pobiera szkielety programów oraz inne potrzebne pliki wymagane w laboratorium o zadanym temacie.
• \"Pomoc na temat skryptu mpi\" Pomoc, którą czytasz w tej chwili.
• \"Wyjście\" Wyjście ze skryptu mpi.
";;
	ssh) 
           $KDIALOG --title "Wygenerowanie klucza publicznego ssh" \
                --warningcontinuecancel "Czy jesteś pewien, że chcesz kontynuować? Jeżeli wciśniesz \"Kontynuuj\", skasuję pliki ${HOME}/.ssh/id_rsa, ${HOME}/.ssh/id_rsa.pub oraz ${HOME}/.ssh/authorized_keys i wygeneruję je od nowa. Wykonam następujące polecenia:

    rm -f ${HOME}/.ssh/id_rsa 2>/dev/null
    rm -f ${HOME}/.ssh/authorized_keys 2>/dev/null
    ssh-keygen -f ${HOME}/.ssh/id_rsa -q -N ""
    cp ${HOME}/.ssh/id_rsa.pub ${HOME}/.ssh/authorized_keys

Wciśnij \"Kontynuuj\" jeżeli jesteś pewien"

	    if [ $? -eq 0 ]; then
	       init_ssh
               if [ -e ${HOME}/.ssh/id_rsa ]; then
                LISTA=""
                echo ${LAB}
                K=`seq 1 15 | tr " " + | tr \t + | tr "\n" +`
                OLD=$IFS
                IFS=+

                for i in $K;do 
                    LISTA="$LISTA ${LAB}-$i ${LAB}-$i on"
                done
                IFS=$OLD
                
                $KDIALOG --title "Ok!" --msgbox "Wygląda na to, że wszystko jest ok. Jeżeli mimo tego przy łączeniu się na inne konto system pyta ciebie o hasło, zawołaj prowadzącego"
                LAB_LIST=$($KDIALOG --title "Zainicjalizować known_hosts?" --checklist "Czy chcesz, żebym teraz spróbował się połączyć z wszystkimi wybranymi poniżej hostami? 
Odznacz te, które są wyłączone.  
Wykonam dla każdego zaznaczonego węzła polecenie:

    ssh-keyscan -t rsa HOSTNAME >> ~/.ssh/known_hosts

Spowoduje to, że węzeł zostanie dodanie do known_hosts.
Jeżeli mimo tego nie możesz się łączyć z hostami bez podawania hasła,
spróbuj wydać polecenia poniżej:
    
    ssh-agent bash
    ssh-add

Wciśnij \"Anuluj\", jeżeli nie chcesz się połączyć z hostami. " ${LISTA})

                if [ $? -eq 0 ]; then
		    for i in $LAB_LIST;do 
			#ssh -o ConnectTimeout=1 $i true
                        ssh-keyscan -t rsa ${i//\"/} >> ~/.ssh/known_hosts
		    done
                fi
               else
                $KDIALOG --title "Oops!" --error "Coś nie wyszło, ale nie wiem co"
               fi
	    else
                true;
	    fi
            true;;
        init) 
           $KDIALOG --title "Inicjalizacja środowiska do pracy" \
                --warningcontinuecancel "Czy jesteś pewien, że chcesz kontynuować? Jeżeli wciśniesz \"Kontynuuj\", skasuję pliki ${HOME}/mpi i wszystko, co się w nich znajduje. Wykonam następujące polecenia:

    cp -r ${HOME}/mpi ${HOME}/mpi-backup-`date +%F`
    rm -rf ${HOME}/mpi 2>/dev/null
    wget ${REPO}/first.tgz

Wciśnij \"Kontynuuj\" jeżeli jesteś pewien"

	    if [ $? -eq 0 ]; then
		init_env
		$KDIALOG --title "OK" --msgbox "Zamknij i otwórz konsolę tekstową lub wydaj polecenie \". ~/.bashrc\""
            fi;;
	lab) 
            IFS=+
            OPT=$($KDIALOG --title "Pobieranie plików do laboratorium" \
            --combobox "Pliki zostaną ściągnięte do katalogu \"mpi\" w katalogu domowym. Wybierz temat laboratorium z listy poniżej" \
            "Wszystkie" \
            ${TEMATY_NAZWY} \
            --$DEFAULT_OPT "Wszystkie" )
#            "Łamanie haseł" \
#            "Zegary logiczne Matterna" \
#            "Wykrywanie zakończenia" \
#            "Skalowanie zdjęć" \
#            "Wyliczanie π metodą Monte Carlo"\
#            "Rozgłaszanie"\
#            "Wykrywanie spójnego stanu globalnego"\
#            "Przelewy bankowe"\
#            "Wykrycie zaginięcia tokenu"
            IFS=" 	"
            case $OPT in
                "Wszystkie")
                    for i in ${TEMATY}
                    do get_lab $i
			if [ ! $? -eq 0 ]; then 
			    $KDIALOG --title "Błąd!" --error "Nie udało mi się ściągnąć pliku. \n Sprawdź połączenie sieciowe; sprawdź także, czy masz dosyć miejsca na dysku (polecenie quota -v)"
			    break
			fi
                    done;;
                *)
                #echo ${TEMATY_ASSOC[$OPT]} :: $OPT
                    if [ ! -z "$OPT" ]; then
			TMP=${TEMATY_ASSOC[$OPT]}
                #echo $TMP
			if [ -z "$TMP" ]; then #niemozliwe
			    $KDIALOG --title "Błąd!" --error "Dziwny błąd; prawdopodobnie błąd w pliku konfiguracyjnym ${HOME}/mpi/.tematy"
			else
			    get_lab $TMP
			    if [ ! $? -eq 0 ]; then 
				$KDIALOG --title "Błąd!" --error "Nie udało mi się ściągnąć pliku. \n Sprawdź połączenie sieciowe; sprawdź także, czy masz dosyć miejsca na dysku (polecenie quota -v)"
			    fi
			fi
                    fi
                echo "wybrałeś $OPT";;
            esac
            true;;
        koniec) exit 0;;
    esac
done
