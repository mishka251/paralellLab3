#include<mpi.h>
#include<iostream>
#include<time.h>
#include<algorithm>
#include<chrono>
#include<locale.h>
#include<Windows.h>
#include<string>

using namespace std;


int* arr, *arr2;//2 массива дл€ сортировки(1 параллельно, 2 последовательно)
const int N = 6 * 1E6;//размер массивов

//функци€ сравнени€ дл€ qsort
int cmp(const void *a1, const void *b1);
//инициализаци€(заполнение массивов)
void init();
//проверка массива на отсортированность
bool arrSorted(int* ar, int n);
//сравнеине массивов
bool arrCmp(int* ar1, int *ar2, int n);
//вывод массива
void printArr(int*a, int n);
//сли€ние двух сортированых массивов в один сортированый
int* sortedArrayMerge(int *a1, int n, int*a2, int m);
//смена кодировки дл€ символа
char changeFunckingCharCode(char c);
//смена кодировки дл€ строчки
string changeFunckingStrCode(string s);



//параллельна€ сортировка
void notParallelSort()
{
	qsort(arr2, N, sizeof(int), cmp);
}

//функци€ дл€ одной итерации параллельной сортировки
void paralleIteration(int trNum, int iterNum, int trCount, int* my_arr, int my_n)
{
	bool us_next = trNum % 2 == iterNum % 2;//тру - в паре со следующий процессом, фалс - с предыдущим

	int parTrNum = -1;
	if (us_next)
		parTrNum = trNum + 1;
	else
		parTrNum = trNum - 1;

	if (parTrNum < 0 || parTrNum >= trCount)//не с кем обмениватьс€
		return;

	int*ar2 = new int[my_n];//буфер от парного процесса

	//всегда поток с меньшим номером отправл€ет свою часть потоку с большим номером
	//затем наоборот
	int id = min(trNum, parTrNum) + 10 * max(trNum, parTrNum);
	MPI_Status sts;
	//пр€мой обмен
	if (us_next)
		MPI_Send(my_arr, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD);
	else
		MPI_Recv(ar2, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD, &sts);

	//обратный
	if (!us_next)
		MPI_Send(my_arr, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD);
	else
		MPI_Recv(ar2, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD, &sts);


	int* all_ar = sortedArrayMerge(my_arr, my_n, ar2, my_n);//общий массив

	//оставл€ем себе или начало или конец
	if (us_next)
		for (int i = 0; i < my_n; i++)
			my_arr[i] = all_ar[i];
	else
		for (int i = 0; i < my_n; i++)
			my_arr[i] = all_ar[i + my_n];


	delete[] ar2;//чистим пам€ть
	delete[] all_ar;

}

//параллельна€ сортировка. номер потока и кол-во получаем из мейна
void parallelSort(int rank, int size)
{
	int my_n = N / size;//элементов в каждом процессе
	if (N%size != 0)//если не поровну
	{
		my_n++;
		if (rank == 0)
		{
			//добавл€ем размер до нужного
			arr = (int*)realloc(arr, sizeof(int)*my_n*size);
			//добавл€ем в арр  чтобы делилось
			for (int i = N; i < my_n*size; i++)
				arr[i] = INT_MAX;
		}
	}
	//массив в процессе
	int *my_arr = new int[my_n];

	//рассылаем с 0 на остальные
	MPI_Scatter(arr, my_n, MPI_INT, my_arr, my_n, MPI_INT, 0, MPI_COMM_WORLD);

	//сортируем на этом процессе
	qsort(my_arr, my_n, sizeof(int), cmp);

	//итерации сортировки
	for (int i = 0; i < size; i++)
		paralleIteration(rank, i, size, my_arr, my_n);

	//собираем со всех процессов в один массив на 0 потоке
	MPI_Gather(my_arr, my_n, MPI_INT, arr, my_n, MPI_INT, 0, MPI_COMM_WORLD);

}

void laba(int argc, char **argv)
{
	//номер процесса, кол-во
	int rank, size;
	//дл€ замеров времени
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
	}

	//последовательна€ сортировка только на 1 процессе
	if (rank == 0)
	{
		start = std::chrono::steady_clock::now();
		notParallelSort();
		end = std::chrono::steady_clock::now();
		no_par_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	if (rank == 0)
		start = std::chrono::steady_clock::now();//запуск расчета на 0

	parallelSort(rank, size);//параллельна€ сортирвока

	if (rank == 0)
	{
		end = std::chrono::steady_clock::now();//конец расчета на 0
		par_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	MPI_Finalize();

	if (rank == 0)
	{

		if (arrSorted(arr, N))
			cout << changeFunckingStrCode("ћассив отсортирован");
		else
			cout << "WTF?";
		cout << endl;

		cout << changeFunckingStrCode("ѕоследовательное врем€ ") << no_par_time << changeFunckingStrCode(" мс") << endl;
		cout << changeFunckingStrCode("ѕараллельное врем€ ") << par_time << changeFunckingStrCode(" мс") << endl;
		cout << changeFunckingStrCode("”скорение ") << no_par_time - par_time << changeFunckingStrCode(" мс") << endl;
		cout << changeFunckingStrCode("—равнение массивов ") << arrCmp(arr, arr2, N) << endl;
		cout << changeFunckingStrCode("»спользовано потоков ") << size << endl;
		cout << changeFunckingStrCode("–азмер массива ") << N << endl;
		system("pause");
	}

}

int main(int argc, char **argv)
{
	laba(argc, argv);

	return 0;
}

char changeFunckingCharCode(char c)
{
	if (c >= 'ј'&&c <= 'я')
		return 128 + (c - 'ј');

	//a 160 п 175
	if (c >= 'а'&&c <= 'п')
		return 160 + (c - 'а');

	//р 224
	if (c >= 'р'&&c <= '€')
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

//функци€ сравнени€ дл€ qsort(тело)
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
		arr2[i] = arr[i] = rand();
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