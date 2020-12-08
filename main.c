#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
// G�nl�k maximum musteri sayisi
#define MAX_MUSTERI 25                    

// Fonksiyon prototiplerini tanimliyoruz
void *musteri(void *num);
void *berber(void *);
void randTrasTipi();
void randMesafe();

//Global degiskenleri tanimliyoruz
static int toplamUcret = 0;
static int fiyatTarife = 0;
static int trastakiMusteriNo = 0;
static int mesafe = 0;
static char trasTipi[50] = "";

// Semaphorlari tanimliyoruz

// Bu semafor i�eride ka� kisinin bekleyebilecegini belirler
sem_t beklemeOdasi;

// Bu semafor berber koltuguna sirayla oturabilmeyi saglar
//Mutual exclusion
sem_t berberKoltugu;

// Cekyat semaforu, bir m�steri gelene kadar berberin yatmasi-uyumasi i�indir
sem_t cekYat;

// Trasi bekle semaforu, berber isini bitirene kadar, m�sterinin beklemesini saglar
sem_t trasiBekle;

// T�m m�sterilerin trasi tamamlanana kadar 0, sonra 1 degerini alir
int tumuTamamlandi = 0;

int main() {
	pthread_t btid;
	pthread_t tid[MAX_MUSTERI];
	long RandSeed;
	int i, musteriSayisi, koltukSayisi;
	int Numarator[MAX_MUSTERI];

	printf("Musteri sayisini giriniz : "); scanf("%d", &musteriSayisi);		//M�steri sayisini aliyoruz, en fazla 25 olarak tanimladik
	printf("Koltuk sayisini giriniz : "); scanf("%d", &koltukSayisi);		//Bekleme koltugu sayisini aliyoruz

	//Maksimum m�steri kontrol� yapiyoruz
	if (musteriSayisi > MAX_MUSTERI)
	{
		printf("Girebileceginiz en yuksek musteri sayisi %d'dir.\n", MAX_MUSTERI);
		exit(-1);
	}
	printf("*****************************************************\n");



	// Numarat�r dizisini olusturuyoruz.
	for (i = 0; i < MAX_MUSTERI; i++) {
		Numarator[i] = i;
	}

	// Semaforlar, degerleri ile init ediliyor
	sem_init(&beklemeOdasi, 0, koltukSayisi);
	sem_init(&berberKoltugu, 0, 1);
	sem_init(&cekYat, 0, 0);
	sem_init(&trasiBekle, 0, 0);

	//Berber threadini olusturuyoruz
	pthread_create(&btid, NULL, berber, NULL);

	//M�steri threadlerini olusturuyoruz
	for (i = 0; i < musteriSayisi; i++) {
		pthread_create(&tid[i], NULL, musteri, (void *)&Numarator[i]);
		sleep(1);
	}

	//M�steri threadleri birbirlerini beklesin diye, onlari join yapiyoruz
	for (i = 0; i < musteriSayisi; i++) {
		pthread_join(tid[i], NULL);
		sleep(1);
	}

	//B�t�n m�steriler bittiginde, isareti 0'dan 1'e aliyoruz
	tumuTamamlandi = 1;
	sem_post(&cekYat); // Berberi eve gitmesi i�in uyandiriyoruz
	pthread_join(btid, NULL);
}

//M�steri fonksiyonu evden �ikma ile baslar, d�kkandan �ikma ile biter
void *musteri(void *numara) {
	int num = *(int *)numara;

	//M�steri evden �ikiyor
	printf("%d numarali musteri berbere gitmek uzere evden cikiyor.\n", num);
	//Random bir mesafe ve bekleme s�resi tanimliyoruz
	randMesafe();
	//M�steri d�kkana vardi, ama daha i�eri girmedi. Geldigi mesafeyi g�steriyoruz
	printf("%d numarali musteri berber dukkanina vardi. Geldigi yol %d KM\n", num, mesafe);

	//M�steri bekleme odasinda bos koltuk olmasini bekliyor ve yer olunca i�eri giriyor
	sem_wait(&beklemeOdasi);
	printf("%d numarali musteri dukkana giriyor.\n", num);

	// M�steri i�eride sirasinin gelmesini bekliyor
	sem_wait(&berberKoltugu);




	// Berber koltugunun bosaldigi i�in kalkan m�steri, bekleme koltugunun bosaldigini bildiriyor
	sem_post(&beklemeOdasi);

	//Sirasi gelen m�steri berberi uyandiriyor
	printf("%d numarali musteri berberi uyandiriyor.\n", num);
	trastakiMusteriNo = num;
	sem_post(&cekYat);		//�ekyatin bosaldigini bildiriyoruz

	// Trasin bitmesini bekliyor
	sem_wait(&trasiBekle);

	// Tras bitti, koltugun bosaldigini bildiriyoruz
	sem_post(&berberKoltugu);
	printf("%d numarali musteri berber dukkanindan cikiyor.\n", num);
}


//Berber fonksiyonu
void *berber(void *junk) {
	//isi tamamlanmayan m�steri oldugu s�rece islem devam eder
	while (!tumuTamamlandi) {

		//Firsat varken uyuyor
		printf("Berber uyuyor\n");
		sem_wait(&cekYat);


		if (!tumuTamamlandi) {

			// Rastgele bir tras tipi ve bekleme s�resi belirliyoruz
			randTrasTipi();
			//Berber tras yapiyor ve trasi bitiriyor
			printf("Berber %d numarali musteriyi %s  ediyor\n", trastakiMusteriNo, trasTipi);
			printf("Berber %d numarali musterinin trasini bitirdi ve %d TL aldi.\n", trastakiMusteriNo, fiyatTarife);

			// Trasi bekle semaforuna trasin bittigini bildir
			sem_post(&trasiBekle);
		}
		else {
			//Berber t�m m�sterilerinin isini bitirdi ve eve gidiyor
			printf("*****************************************************\n");
			printf("Berber, dukkani kapatti ve eve gidiyor. %d TL ciro yapti \n", toplamUcret);
			//tumuTamamlandi = 0;

			//Tekrar main fonksiyonunu �agirarak bastan baslayabiliriz
			//main();
		}
	}
}

void randTrasTipi() {
	int len;

	srand(time(NULL));	//Rastgele bir sayi beliliyoruz
	len = rand() % 3; // Sayi 0 , 1 , 2 olabilir

	//sayiya g�re tras tipi ve �cret belirliyoruz
	switch (len)
	{
	case 0:
		//printf("Sac trasi\n");
		strcpy(trasTipi, "Sac trasi");
		fiyatTarife = 35;
		toplamUcret = toplamUcret + fiyatTarife;
		break;
	case 1:
		//printf("Sakal trasi\n");
		strcpy(trasTipi, "Sakal trasi");
		fiyatTarife = 20;
		toplamUcret = toplamUcret + fiyatTarife;
		break;
	case 2:
		//printf("Sac Sakal karisik\n");
		strcpy(trasTipi, "Sac Sakal karisik trasi");
		fiyatTarife = 50;
		toplamUcret = toplamUcret + fiyatTarife;
		break;
	}
	sleep(len);
}



void randMesafe() {
	int len;

	// Rastgele bir sayi belirliyoruz

	srand(time(NULL));
	len = rand() % 20;	// sayi 0-20 arasi olabilir
	sleep(len);
	mesafe = len;	//M�steri bu yoldan gelecek


}


