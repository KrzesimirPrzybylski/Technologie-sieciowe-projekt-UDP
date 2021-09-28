#include "protokol.h"
#include "klient.h"
#include "serwer.h"

int main()
{
	int menu;
	do
	{
		std::cout << "Uruchom serwer -> 1\nUruchom program klienta -> 2\n" << std::flush;
		std::cin >> menu;
	} while (menu > 2);

	if (menu == 1)
	{
		serwer s;
		s.run();
	}
	else if (menu == 2)
	{

		klient k;
		k.run();
	}

	return 0;
}
