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

//������� ��������� ��� qsort(���������)
int cmp(const void *a1, const void *b1);

//������������ ����������(���������� ����� ������
void notParallelSort()
{
	qsort(arr2, N, sizeof(int), cmp);
}


//�������������(���������� ��������)
void init();
//�������� ������� �� �����������������
bool arrSorted(int* ar, int n);
//��������� ��������
bool arrCmp(int* ar1, int *ar2, int n);
//����� �������
void printArr(int*a, int n);
//������� ���� ������������ �������� � ���� ������������(���������)
int* sortedArrayMerge(int *a1, int n, int*a2, int m);

//������� ��� ����� �������� ������������ ����������
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
	//������ ����� � ������� ������� ���������� ���� ����� ������ � ������� �������
	//����� ��������
	int id = min(trNum, parTrNum) + 10 * max(trNum, parTrNum);
	MPI_Status sts;
	if (us_next)
		MPI_Send(my_arr, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD);
	else
		MPI_Recv(ar2, my_n, MPI_INT, parTrNum, id, MPI_COMM_WORLD, &sts);

	//����� �������
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


	delete[] ar2;//������ ������
	delete[] all_ar;

}

//������������ ����������. ����� ������ � ���-�� �������� �� �����
void parallelSort(int rank, int size)
{
	int my_n = N / size;//��������� � ������ ��������
	if (N%size != 0)//���� �� �������
	{
		my_n++;
		if (rank == 0)
		{
			int* arrTmp = new int[my_n*size];
			for (int i = 0; i < N; i++)
				arrTmp[i] = arr[i];
			//��������� � ���  ����� ��������
			//arr = (int*)realloc(arr, my_n*size);
			for (int i = N; i < my_n*size; i++)
				arrTmp[i] = INT_MAX;
			arr = arrTmp;
		}
	}

	int *my_arr = new int[my_n];
	//if(rank==0)
	//printArr(arr, my_n*size);

	//��������� � 0 �� ���������
	MPI_Scatter(arr, my_n, MPI_INT, my_arr, my_n, MPI_INT, 0, MPI_COMM_WORLD);


	//��������� �� ���� ��������
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

	//�������� �� ���� ��������� � ���� ������ �� 0 ������
	MPI_Gather(my_arr, my_n, MPI_INT, arr, my_n, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

}

char changeFunckingCharCode(char c);
string changeFunckingStrCode(string s);

void laba(int argc, char **argv)
{
	//����� ��������, ���-��
	int rank, size;
	//��� ������� �������
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	long long par_time = 0, no_par_time = 0;


	MPI_Init(&argc, &argv);
	//��������
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//�� 0 �������� ������� �������
	if (rank == 0)
	{
		init();
		//printArr(arr, N);
	}

	//���������������� ���������� ������ �� 1 ��������
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
		start = std::chrono::steady_clock::now();//������ ������� �� 0

	parallelSort(rank, size);//������������ ����������

	//MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
	{
		end = std::chrono::steady_clock::now();//����� ������� �� 0
		par_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	MPI_Finalize();

	if (rank == 0)
	{

		if (arrSorted(arr, N))
			cout <<  changeFunckingStrCode("������ ������������");
		else
			cout << "WTF?";
		cout << endl;

		cout << changeFunckingStrCode( "���������������� ����� " )<< no_par_time << changeFunckingStrCode(" ��") << endl;
		cout <<changeFunckingStrCode( "������������ ����� ") << par_time << changeFunckingStrCode( " ��") << endl;
		cout << changeFunckingStrCode( "��������� " )<< no_par_time - par_time <<  changeFunckingStrCode(" ��") << endl;
		cout << changeFunckingStrCode( "��������� �������� " )<< arrCmp(arr, arr2, N) << endl;
		cout << changeFunckingStrCode( "������������ ������� " )<< size << endl;
		cout << changeFunckingStrCode( "������ ������� " )<< N << endl;
		system("pause");
	}

}


void test(int argc, char **argv)
{
	//����� ��������, ���-��
	int rank, size;
	

	MPI_Init(&argc, &argv);
	//��������
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


	if (rank == 0)
	{
		//SetConsoleCP(1251);
		//SetConsoleOutputCP(1251);
		//printArr(arr, N);
	
		

		for (int i = 128; i < 256; i++)
			cout << i << "  " << char(i) << endl;

		string alph = "��������������������������������93��98";
		//for (int i = 0; i < alph.length(); i++)
			//cout << 255+(int)alph[i] << " " << alph[i] << "  " << 255+(int)changeFunckingCharCode(alph[i]) <<"  "<< changeFunckingCharCode(alph[i]) << endl;
		cout << alph << endl;
		cout << changeFunckingStrCode(alph) << endl;

		//for (int c = 128; c <= 255; c++)
			//cout << (char)c<<" "<<(int)c << " " << 255+(int)changeFunckingCharCode(c) << " " << changeFunckingCharCode(c) << endl;
		//cout << endl;

		//for (char c = '�'; c <= '�'; c++)
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
	if (c >= '�'&&c <= '�')
		return 128 + (c - '�');

	//a 160 � 175
	if (c >= '�'&&c <= '�')
		return 160 + (c - '�');

	//� 224
	if (c >= '�'&&c <= '�')
		return 224 + (c - '�');

	return c;
}

string changeFunckingStrCode(string s)
{
	string res = s;
	for (int i = 0; i < s.length(); i++)
		res[i] = changeFunckingCharCode(s[i]);

	return res;
}

//������� ��������� ��� qsort(����)
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