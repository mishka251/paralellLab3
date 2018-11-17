#include<mpi.h>
#include<iostream>
#include<time.h>
#include<algorithm>
#include<chrono>
#include<locale.h>
#include<Windows.h>
#include<string>


using namespace std;


int* arr;
int* arr2;
const int N = 6*1E6;

//функция сравнения для qsort(заголовок)
int cmp(const void *a1, const void *b1);

//параллельная сортировка(возвращает время работы
void notParallelSort()
{
	qsort(arr2, N, sizeof(int), cmp);
}


//инициализация(заполнение массивов)
void init();
//проверка массива на отсортированность
bool arrSorted(int* ar, int n);
//сравнеине массивов
bool arrCmp(int* ar1, int *ar2, int n);
//вывод массива
void printArr(int*a, int n);
//слияние двух сортированых массивов в один сортированый(заголовок)
int* sortedArrayMerge(int *a1, int n, int*a2, int m);

//функция для одной итерации параллельной сортировки
void mySort(int trNum, int iterNum, int trCount, int* my_arr, int my_n)
{

	bool us_next = trNum % 2 == iterNum % 2;

	int parTrNum = -1;
	if (us_next)
		parTrNum = trNum + 1;
	else
		parTrNum = trNum - 1;

	if (parTrNum < 0 || parTrNum >= trCount)
		return;

	int*ar2 = new int[my_n];
	//всегда поток с меньшим номером отправляет свою часть потоку с большим номером
	//затем наоборот
	int id = min(trNum, parTrNum) + 10 * max(trNum, parTrNum);
	MPI_Status sts;
	if (us_next)
		MPI_Send(my_arr, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD);
	else
		MPI_Recv(ar2, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD, &sts);

	//потом обратно
	if (!us_next)
		MPI_Send(my_arr, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD);

	else
		MPI_Recv(ar2, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD, &sts);


	int* all_ar = sortedArrayMerge(my_arr, my_n, ar2, my_n);

	if (us_next)
		for (int i = 0; i < my_n; i++)
			my_arr[i] = all_ar[i];
	else
		for (int i = 0; i < my_n; i++)
			my_arr[i] = all_ar[i + my_n];


	delete[] ar2;//чистим память
	delete[] all_ar;

}

//параллельная сортировка. номер потока и кол-во получаем из мейна
void parallelSort(int rank, int size)
{
	int my_n = N / size;//элементов в каждом процессе
	if (N%size != 0)//если не поровну
	{
		my_n++;
		if (rank == 0)
		{
			int* arrTmp = new int[my_n*size];
			for (int i = 0; i < N; i++)
				arrTmp[i] = arr[i];
			//добавляем в арр  чтобы делилось
			//arr = (int*)realloc(arr, my_n*size);
			for (int i = N; i < my_n*size; i++)
				arrTmp[i] = INT_MAX;
			arr = arrTmp;
		}
	}

	int *my_arr = new int[my_n];
	//if(rank==0)
	//printArr(arr, my_n*size);

	//рассылаем с 0 на остальные
	MPI_Scatter(arr, my_n, MPI_INT, my_arr, my_n, MPI_INT, 0, MPI_COMM_WORLD);


	//сортируем на этом процессе
	qsort(my_arr, my_n, sizeof(int), cmp);

	//MPI_Barrier(MPI_COMM_WORLD);

	for (int i = 0; i < size; i++)
	{
		//cout << "iter"<<i<<" tr" << rank << endl;
		//printArr(my_arr, my_n);

		mySort(rank, i, size, my_arr, my_n);
		MPI_Barrier(MPI_COMM_WORLD);
	}

	//MPI_Barrier(MPI_COMM_WORLD);

	//cout << "end " << " tr" << rank << endl;
	//printArr(my_arr, my_n);

	//собираем со всех процессов в один массив на 0 потоке
	MPI_Gather(my_arr, my_n, MPI_INT, arr, my_n, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

}

char changeFunckingCharCode(char c);
string changeFunckingStrCode(string s);

void laba(int argc, char **argv)
{
	//номер процесса, кол-во
	int rank, size;
	//для замеров времени
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	long long par_time = 0, no_par_time = 0;


	MPI_Init(&argc, &argv);
	//получили
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//на 0 процессе создали массивы
	if (rank == 0)
	{
		init();
		//printArr(arr, N);
	}

	//последовательная сортировка только на 1 процессе
	if (rank == 0)
	{
		start = std::chrono::steady_clock::now();
		notParallelSort();
		end = std::chrono::steady_clock::now();
		//cout << "sorted" << endl;
		//printArr(arr2, N);
		no_par_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}


	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
		start = std::chrono::steady_clock::now();//запуск расчета на 0

	parallelSort(rank, size);//параллельная сортирвока

	//MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
	{
		end = std::chrono::steady_clock::now();//конец расчета на 0
		par_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	MPI_Finalize();

	if (rank == 0)
	{

		if (arrSorted(arr, N))
			cout <<  changeFunckingStrCode("Массив отсортирован");
		else
			cout << "WTF?";
		cout << endl;

		cout << changeFunckingStrCode( "Последовательное время " )<< no_par_time << changeFunckingStrCode(" мс") << endl;
		cout <<changeFunckingStrCode( "Параллельное время ") << par_time << changeFunckingStrCode( " мс") << endl;
		cout << changeFunckingStrCode( "Ускорение " )<< no_par_time - par_time <<  changeFunckingStrCode(" мс") << endl;
		cout << changeFunckingStrCode( "Сравнение массивов " )<< arrCmp(arr, arr2, N) << endl;
		cout << changeFunckingStrCode( "Использовано потоков " )<< size << endl;
		cout << changeFunckingStrCode( "Размер массива " )<< N << endl;
		system("pause");
	}

}


void test(int argc, char **argv)
{
	//номер процесса, кол-во
	int rank, size;
	

	MPI_Init(&argc, &argv);
	//получили
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


	if (rank == 0)
	{
		//SetConsoleCP(1251);
		//SetConsoleOutputCP(1251);
		//printArr(arr, N);
	
		

		for (int i = 128; i < 256; i++)
			cout << i << "  " << char(i) << endl;

		string alph = "абвгдежзийклмнопрстуфхцчущяючбяц93пр98";
		//for (int i = 0; i < alph.length(); i++)
			//cout << 255+(int)alph[i] << " " << alph[i] << "  " << 255+(int)changeFunckingCharCode(alph[i]) <<"  "<< changeFunckingCharCode(alph[i]) << endl;
		cout << alph << endl;
		cout << changeFunckingStrCode(alph) << endl;

		//for (int c = 128; c <= 255; c++)
			//cout << (char)c<<" "<<(int)c << " " << 255+(int)changeFunckingCharCode(c) << " " << changeFunckingCharCode(c) << endl;
		//cout << endl;

		//for (char c = 'А'; c <= 'Я'; c++)
			//cout << changeFunckingCharCode(c);
		//cout << endl;

			
		system("pause");
	}
	MPI_Finalize();
}





int main(int argc, char **argv)
{
	laba(argc, argv);
	
	return 0;
}

char changeFunckingCharCode(char c)
{
	if (c >= 'А'&&c <= 'Я')
		return 128 + (c - 'А');

	//a 160 п 175
	if (c >= 'а'&&c <= 'п')
		return 160 + (c - 'а');

	//р 224
	if (c >= 'р'&&c <= 'я')
		return 224 + (c - 'р');

	return c;
}

string changeFunckingStrCode(string s)
{
	string res = s;
	for (int i = 0; i < s.length(); i++)
		res[i] = changeFunckingCharCode(s[i]);

	return res;
}

//функция сравнения для qsort(тело)
int cmp(const void *a1, const void *b1)
{
	int a = *(int*)a1;
	int b = *(int*)b1;
	if (a > b)
		return 1;
	if (a < b)
		return -1;
	return 0;
}


void init()
{
	arr = new int[N];
	arr2 = new int[N];
	srand(unsigned(time(0)));
	for (int i = 0; i < N; i++)
		arr2[i] = arr[i] = rand() % 10;
}

bool arrSorted(int* ar, int n)
{
	for (int i = 0; i < n - 1; i++)
		if (ar[i] > ar[i + 1])
			return false;
	return true;
}

bool arrCmp(int* ar1, int *ar2, int n)
{
	for (int i = 0; i < n; i++)
		if (ar1[i] != ar2[i])
			return false;
	return true;
}

void printArr(int*a, int n)
{
	for (int i = 0; i < n; i++)
		cout << a[i] << " ";
	cout << endl;
}

int* sortedArrayMerge(int *a1, int n, int*a2, int m)
{
	int* all_ar = new int[m + n];

	int i = 0, j = 0;
	for (int k = 0; k < m + n; k++)
	{
		if (i == n)
		{
			all_ar[k] = a2[j];
			j++;
			continue;
		}
		if (j == m)
		{
			all_ar[k] = a1[i];
			i++;
			continue;
		}
		if (a1[i] > a2[j])
		{
			all_ar[k] = a2[j];
			j++;
		}
		else
		{
			all_ar[k] = a1[i];
			i++;
		}

	}
	return all_ar;
}