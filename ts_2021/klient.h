#pragma once
#include "protokol.h"

int odsw = 0;
class klient
{
public:
	klient() : portSerwera(0), portHosta(0), identyfikatorSesji(0), finish(0) {}

	sf::IpAddress adresSerwera;
	unsigned short portSerwera;
	unsigned short portHosta;
	sf::UdpSocket socket;
	sf::Uint8 identyfikatorSesji;
	bool finish;
	


	void run()
	{
		std::string temp;
		std::cout << "Podaj IP serwera: ";
		std::cin >> temp;
		adresSerwera = sf::IpAddress(temp.c_str());
		std::cout << "Podaj port serwera: ";
		std::cin >> portSerwera;

		if (socket.bind(portHosta) != sf::Socket::Done)
		{
			std::cout << "Blad bindowania" << std::endl;
			std::cin.ignore(2);
			exit(-1);
		}
		else
		{
			socket.setBlocking(false);
			system("cls");
			sf::IpAddress ip_addr = sf::IpAddress::getLocalAddress();
			std::string ip_addr_str = ip_addr.toString();
			int port = socket.getLocalPort();
			std::cout << "Klient " << ip_addr_str << ":" << port << std::endl;
			std::cout << "ADRES serwera: " << adresSerwera.toString() << ":" << portSerwera << std::endl;
		}

		// uzyskanie id sesji
		std::cout << "Uzyskiwanie identyfikatora sesji\n";
		pakiet t;
		t.komunikaty.ID = 1;
		sendpakiet(t);
		

		//odbiór danych
		while (!finish)
		{
			
			//std:: thread thr(odswiez); 
			//std::thread thr2(menu1);
			
			//thr.join();
			pakiet aktualny_pakiet;
			sf::Socket::Status status;
			for (int i = 0; i < 10; i++)
			{
				sf::sleep(sf::milliseconds(100));
				
				status = receivepakiet(aktualny_pakiet);

				if (status == sf::Socket::Done)
				{

					if (aktualny_pakiet.komunikaty.ACK)
					{
						identyfikatorSesji = aktualny_pakiet.identyfikator;
						if (identyfikatorSesji == 0) {
							std::cout << "Proba podlaczenia zbyt duzej liczby klientow!";
							sf::sleep(sf::milliseconds(3000));
							system("cls");
							run();
						}
					}
					
					if (aktualny_pakiet.komunikaty.ZAPR)
					{
						int w;
						std::cout << "zaproszenie do polaczenia od drugiego klienta   \njesli przyjmujesz zaproszenie wcisnij 1,   \njesli nie przyjmujesz zaproszenia wcisnij 0" << std::endl;
						std::cin >> w;
						pakiet odp;
						if (w)
						{
							odp.komunikaty.Z_TAK = 1;
							sendpakiet(odp);
						}
						else
						{
							odp.komunikaty.Z_NIE = 1;
							sendpakiet(odp);
						}
					}

					if (aktualny_pakiet.dlugoscDanych > 0)
					{
						std::cout << "wiadomosc od drugiego klienta: " << aktualny_pakiet.getText() << std::endl;
					}
				}
				aktualny_pakiet.clear();
			}

			// interfejs klienta
			{

				unsigned int menu = 0;
				std::cout << " Wyslij zaproszenie -> 1\n Wyslij wiadomosc -> 2\n Wyswitl ID -> 3\n Odswiez -> 4\n Zakoncz -> 5\n" << std::endl;
				std::cin >> menu;
				if (std::cin.fail()) { std::cin.clear(); menu = 99; std::cin.ignore(); }

				switch (menu)
				{
				case 1: {
					pakiet kom;
					kom = pakiet();
					kom.komunikaty.ZAPR = 1;
					sendpakiet(kom);

					aktualny_pakiet.clear();
					
					while (!status == sf::Socket::Done) {
						sf::sleep(sf::milliseconds(1000));
						status = receivepakiet(aktualny_pakiet);
					}

					aktualny_pakiet.clear();
					status = receivepakiet(aktualny_pakiet);
					while (!status == sf::Socket::Done) {
						sf::sleep(sf::milliseconds(1000));
						status = receivepakiet(aktualny_pakiet);
						if (aktualny_pakiet.komunikaty.Z_TAK && aktualny_pakiet.komunikaty.Z_NIE)
						{
							std::cout << "brak komunikacji z drugim klientem" << std::endl;
						}
						else if (aktualny_pakiet.komunikaty.Z_TAK)
						{
							std::cout << "drugi klient przyjal zaproszenie" << std::endl;
						}
						else if (aktualny_pakiet.komunikaty.Z_NIE)
						{
							std::cout << "drugi klient odrzucil zaproszenie" << std::endl;
						}
					}
					break;
				}
				case 2: {
					pakiet kom;
					std::string wiadomosc;
					char* temp = nullptr;
					std::cout << "Wpisz wiadomosc: ";
					//std::cin.sync();
					std::cin.ignore();
					std::getline(std::cin, wiadomosc);
					//std::cin >> wiadomosc;
					if (std::cin.fail()) { std::cin.clear(); }
					kom = pakiet();
					kom << wiadomosc;
					sendpakiet(kom);
					break;
				}
				case 3: {
					std::cout << "Identyfikator sesji: " << (unsigned int)identyfikatorSesji << std::endl;
					break;
				}
				case 4: {
					std::cout << std::flush;
					break;
				}
				case 5: {
					pakiet kom;
					kom.komunikaty.KON = 1;
					sendpakiet(kom);
					finish = 1;
					break;
				}
				default: {
					std::cout << "blad " << std::endl;
					break;
				}
				}
			}
		
			
		}
		// wy³¹czenie programu po odebraniu ostatniego ACK
		pakiet a;
		socket.setBlocking(true);
		receivepakiet(a);

		sf::sleep(sf::seconds(5));
	}


	// funkcja wysy³aj¹ca pakiet
	void sendpakiet(pakiet p)
	{
		p.identyfikator = identyfikatorSesji;
		// zamiana pakietu na tablicê char
		char* dane = p.toCharArray();

		// wys³anie do serwera tablicy char 
		auto status = socket.send(dane, p.getSize(), adresSerwera, portSerwera);
		if (status != sf::Socket::Done)
		{
			std::cout << "error: " << status << std::endl;
		}
		delete[] dane;
	}

	// funkcja odbieraj¹ca pakiet
	sf::Socket::Status receivepakiet(pakiet& p)
	{
		char* bufor = new char[rozmiarBufora];
		size_t odebrano; // liczba odebranych bajtów

		sf::IpAddress nadawca; // adres IP nadawcy
		unsigned short portNadawcy; // port nadawcy
		sf::Socket::Status status;

		// odebranie pakietu
		status = socket.receive(bufor, rozmiarBufora, odebrano, nadawca, portNadawcy);

		if (status != sf::Socket::Done)
		{
			if (status != sf::Socket::NotReady)
				std::cout << "error: " << status << std::endl;
		}
		else
		{
			// wype³nienie pakietu p odebranymi danymi
			p.fromCharArray(bufor, odebrano);

			// je¿eli odebrany pakiet nie jest potwierdzeniem (ACK) to odsy³ane do serwera jest potwierdzenie odebrania danych
			if (p.komunikaty.ACK == 0)
			{
				pakiet ack; ack.komunikaty.ACK = 1;
				sendpakiet(ack);
			}
		}
		delete[] bufor;
		return status;
	}
};


