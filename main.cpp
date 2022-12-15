#include <iostream>
#include <string>
#include <pthread.h>
#include <openssl/evp.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <string.h>
#include <unistd.h>


#define MAX_LNUM 99    //MAX liczba z lewej strony
#define MAX_RNUM 99    //MAX liczba z prawej strony

using namespace std;


// Klasa przechowujaca dane uzytkownikow
class data{
    public:
        int PID;
        string password;
        string mail;
        string user;
};

//Funkcja wyswietlajaca wszystkie znalezione do tej pory hasla
void show(vector<data*>&matches){
    for(int i = matches.size() -1;i >= 0; --i)
        cout << "Password for " << matches[i]->mail << " is " << matches[i]->password << endl;
}

// Funkcja powielona z https://kcir.pwr.edu.pl/~krzewnicka/?page=haszowanie
// wprowadzono kilka zmian - dostosowanie do jezyka c++
void bytes2md5(string *word, int len, char *md5buf) {
    const char* data = word->c_str();
	EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
	const EVP_MD *md = EVP_md5();
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len, i;
	EVP_DigestInit_ex(mdctx, md, NULL);
	EVP_DigestUpdate(mdctx, data, len);
	EVP_DigestFinal_ex(mdctx, md_value, &md_len);
	EVP_MD_CTX_free(mdctx);
	for (i = 0; i < md_len; i++) {
		snprintf(&(md5buf[i * 2]), 16 * 2, "%02x", md_value[i]);
	}
}

// Inicjalizacja slownika
// [in] name - nazwa pliku slownika
// [in] dict - wskaznik do kontenera przechowujacego slowa
void dict_init(const char* name,vector<string>* dict){
    ifstream file; 
    string buff;

    file.open(name);
    while(file >> buff)
        dict->insert(dict->begin(),buff);
    file.close();
}

// Inicjalizacja danych uzytkownikow
// [in] name - plik tekstowy z haslami
// [in] dict - wzkaznik do kontenera przechowujacego struktury
void pass_init(const char* name,vector<data*>* dict){
    ifstream file; 
    string buff;
    string token;
    stringstream ss;
    vector<string> buff_vect;


    file.open(name);
    do{
        if(getline(file,buff)){     //Wczytanie calej linii z danymi
            stringstream ss(buff);  //wczytanie linii do strumienia
            dict->insert(dict->begin(),new data); //utworzenie nowego wpisu do dynamicznego kontenera (na poczatku)
            while(ss){              // Petla az skoncza sie dane
                ss >> token;        // Wpisywanie kolejnych slow odzielonych znakami bialymi do zmiennej
                if(!ss)             // Upewnienie sie przed zrzutem pamieci
                    break;
                buff_vect.insert(buff_vect.begin(),token);  // Wstawienie slowa do tymczasowego kontenera
            }                                               // Ustawienie kolejnych danych do struktury
            dict->front()->PID = stoi(buff_vect.back());
            buff_vect.pop_back();
            dict->front()->password = buff_vect.back();
            buff_vect.pop_back();
            dict->front()->mail = buff_vect.back();
            buff_vect.pop_back();
            while(!buff_vect.empty()){                      // Wpisanie pozostalosci do ostataniego rekordu - uzytkownicy z bialymi znakami
                dict->front()->user.append(buff_vect.back());
                dict->front()->user.append(" ");
                buff_vect.pop_back();
            }
        }
    }while(buff != "");     // Zakonczenie czytania z pliku
    file.close();
}

// Lamanie hasel z malych liter i bez cyfr
// [in] inf     - przekazany kontener z danymi
// [in] words   - przekazany kontener slownika
// [in] results - przekazany kontener ze zlamanymi haslami
void test1(vector<data*>&inf,vector<string>&words,vector<data*>&results){
    string compare = "";    // Zmienna tymczasowa
    char buf[33];           // Tymczasowa tablica, gdzie zapisane zostanie zahaszowane slowo
    for(int i = inf.size() -1; i >= 0;--i){         // iteracja po kolejnych kontach
       for(int j = words.size()-1; j >= 0;--j){     // iteracja po kolejnych slowach
            bytes2md5(&words[j],words[j].length(),buf);     // haszowanie
            compare += buf;                                 // konwersja char* do string
            if(compare == inf[i]->password){                // w przypadku znalezienia hasla tworzy odpowiedni wpis w kontenerze
                results.insert(results.begin(),new data);
                results.front()->mail = inf[i]->mail;
                results.front()->PID = inf[i]->PID;
                results.front()->user = inf[i]->user;
                results.front()->password = words[j];
                j = -1;                                     // koniec szukania dla znalezionego slowa
            }
            compare.clear();                                // Czyszczenie zmiennej
       }
    }
}


// Lamanie hasel z malych liter z cyframi do 99
// [in] inf     - przekazany kontener z danymi
// [in] words   - przekazany kontener slownika
// [in] results - przekazany kontener ze zlamanymi haslami
void test2(vector<data*>&inf,vector<string>&words,vector<data*>&results){
    string compare = "";            // zmienna porownywana
    string result = "";             // zmienna po zahaszowaniu
    char buf[33];                   // bufor
    
    for(int i = inf.size() -1; i >= 0;--i){                         // iteracja po kontach   
        for(int j = words.size()-1; j >= 0;--j){                    // iteracja po slowach
            for(int n = -1; n < MAX_LNUM + 2; ++n){                 // iteracja po cyfrach z przodu slowa
                for(int g = -1; g < MAX_RNUM + 1 ; ++g){            // iteracja po cyfrach z tylu slowa
                    if(n == -1 && g == -1)                          // przypadek graniczny potrzebny do ustawienia braku cyfry z jednej strony (latwiej polaczyc test1 i test 2 :( ))
                        break;
                    if(n == (MAX_LNUM + 1) && g > -1){              // ustawienia cyfr tylko na koncu slowa           
                        compare += words[j];
                        compare += to_string(g);
                    }
                    if(n > -1 && n < (MAX_LNUM + 1) && g == -1){    // ustawianie cyfr tylko na poczatku slowa
                        compare+= to_string(n);
                        compare += words[j];
                    }
                    if(n >= 0 && n < (MAX_RNUM + 1) && g >= 0){     // ustawianie cyfr po obydwu stronach
                        compare += to_string(n);
                        compare += words[j];
                        compare += to_string(g);
                    }
                    bytes2md5(&compare,compare.length(),buf);       // haszowanie slow
                    result += buf;                                  // konwersja char* na string
                    if(result == inf[i]->password){                 // w przypadku znalezienia hasla tworzy odpowiedni wpis w kontenerze
                    results.insert(results.begin(),new data);
                    results.front()->mail = inf[i]->mail;
                    results.front()->PID = inf[i]->PID;
                    results.front()->user = inf[i]->user;
                    results.front()->password = compare;
                    n = MAX_RNUM + 2;                               // zakonczenie petli cyfr
                    g = MAX_LNUM + 1;                    
                    j = -1;                                         // zakonczenie petli slow dla danego hasla
                    }
                result.clear();                                     // czyszczenie zmiennych
                compare.clear();
                }
            }
        }
    }
}


int main(){
    vector<data*> inf;              // kontener z danymi uzytkownikow
    vector<string> words;           // kontener ze slownikiem 
    vector<data*> matches;          // kontener ze zlamanymi haslami
    const char* pass = "hasla.txt", * dic_file = "slownik.txt";         // W koncowym programie beda to argumenty wywolania albo dane z cin

    pass_init(pass,&inf);          
    dict_init(dic_file,&words);
    test1(inf,words,matches);
    test2(inf,words,matches);
    show(matches);

    return 0;
}