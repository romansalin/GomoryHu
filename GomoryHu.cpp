#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace std;

// количество вершин
const int sz = 6;

// матрица смежности графа (задаем константой, так визуально лучше 
// воспринимается). В функцию построения дерева все же передается 
// "вектор" x "вектор" для универсальности
const int graph[sz][sz] = 
{
	{ 0, 1, 7, 0, 0, 0 },
	{ 1, 0, 1, 3, 2, 0 },
	{ 7, 1, 0, 0, 4, 0 },
	{ 0, 3, 0, 0, 1, 6 },
	{ 0, 2, 4, 1, 0, 2 },
	{ 0, 0, 0, 6, 2, 0 }
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Гомори-Ху
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct Edge;
struct Vertex;

typedef vector<int>     Row;        // строка матрицы смежности
typedef vector<Row>     Matrix;     // матрица смежности
typedef vector<Vertex*> VertexList; // список вершин
typedef vector<Edge*>   EdgeList;   // список ребер

// ----------------------------------------------------------------------------

// Вершина или группа вершин

struct Vertex
{
	int id; // идентификатор вершины, нужен для вывода на экран лога
	        // работы алгоритма, а так же в конце для сортировки
	        // вершин конечного дерева в порядке исходных вершин
	        // (чтобы итоговая матрица смежности соответс. исходной)

	VertexList  group;  // вершины в группе (пусто для простой вершины)
	EdgeList    edges;  // список ребер

	int     flag;       // вспомогательные поля, используются при поиске
	Vertex* parent;     // в ширину и еще кое-где для упрощения жизни

	// конструкторы простой и групповой вершин
	Vertex(int id) : id(id) { }
	Vertex(const VertexList &group) : id(-1), group(group) { }
};

// ----------------------------------------------------------------------------

// Ребро (т.к. ребра находятся в списках у вершин, то в ребре указывается
// только вторая вершина; соответственно, во второй вершине указывается
// аналогичное ребро в обратную сторону)

struct Edge
{
	Vertex* vertex; // смежная вершина
	int c;          // пропускная способность
	int f;          // поток

	Edge(Vertex* vertex, int c) : vertex(vertex), c(c), f(0) { }
};

// ----------------------------------------------------------------------------

// результат поиска макс. потока

struct MinCut
{
	int        f;  // макс. поток между s и t
	Vertex*    s;  // указатель на s
	Vertex*    t;  // указатель на t
	VertexList A;  // "отрезанная" половина, содержащая s
	VertexList B;  // "отрезанная" половина, содержащая t

	MinCut(Vertex* s, Vertex* t) : s(s), t(t), f(0) {}
};

// ----------------------------------------------------------------------------

// Поиск вершины в списке, возвращается итератор (указатель)

VertexList::iterator findVertex(VertexList &set, Vertex* v) 
{
	VertexList::iterator i = set.begin();
	VertexList::iterator j = set.end();
	while(i != j && (*i) != v) i++;
	return i;
}

// ----------------------------------------------------------------------------

// разность множеств
	
VertexList setMinus(VertexList &set1, VertexList &set2) 
{
	// изначально результат пуст
	VertexList result;
	for each(Vertex* v in set1)
	{
		// если вершина из левого не найдена в правом, добавляем в результат
		if(findVertex(set2, v) == set2.end()) result.push_back(v);
	}
	return result;
}

// ----------------------------------------------------------------------------

// пересечение множеств
	
VertexList setMul(VertexList &set1, VertexList &set2) 
{
	// изначально результат пуст
	VertexList result;
	for each(Vertex* v in set2)
	{
		// если вершина из правого найдена в левом, добавляем в результат
		if(findVertex(set1, v) != set1.end()) result.push_back(v);
	}
	return result;
}

// ----------------------------------------------------------------------------

// Поиск ребра в списке, возвращается итератор (указатель)

EdgeList::iterator findEdge(Vertex* v1, Vertex* v2) 
{
	EdgeList::iterator i = v1->edges.begin();
	EdgeList::iterator j = v1->edges.end();
	while(i != j && (*i)->vertex != v2) i++;
	return i;
}

// ----------------------------------------------------------------------------

// Создание ребра V1 --[c]--> V2
// Если addReverse == true - создается и V2 --[c]--> V1
// Если ребро уже имеется, то c добавляется к его весу

void addEdge(Vertex* v1, Vertex* v2, const int c, bool addReverse = true)
{
	EdgeList::iterator i = findEdge(v1, v2);
	if(i != v1->edges.end()) (*i)->c += c;
	else v1->edges.push_back(new Edge(v2, c));

	if(!addReverse) return;
	
	i = findEdge(v2, v1);
	if(i != v2->edges.end()) (*i)->c += c;
	else v2->edges.push_back(new Edge(v1, c));
}

// ----------------------------------------------------------------------------

// Получение ребра (если есть) от V1 к V2

Edge* getEdge(Vertex* v1, Vertex* v2)
{
	EdgeList::iterator i = findEdge(v1, v2);
	return (i == v1->edges.end() ? NULL : *i);
}

// ----------------------------------------------------------------------------

// Уничтожение ребра из V1 в V2

void deleteEdge(Vertex* v1, Vertex* v2)
{
	EdgeList::iterator i = findEdge(v1, v2);
	if(i == v1->edges.end()) return;

	delete (*i);
	v1->edges.erase(i);
}

// ----------------------------------------------------------------------------

// Уничтожение вершины v в множестве set с удалением ребер
	
void deleteVertex(VertexList &set, Vertex* v) 
{
	VertexList::iterator i = findVertex(set, v);
	if(i == set.end()) return;

	// перебираем ребра, связанные с вершиной
	for each(Edge* e in v->edges) 
	{
		// уничтожаем встречное ребро, после чего удаляем прямое
		deleteEdge(e->vertex, v);
		delete e;
	}

	// удаляем вершину
	delete (*i);
	set.erase(i);
}

// ----------------------------------------------------------------------------

// Уничтожение графа (всех вершин и ребер в множестве)

void deleteVertexList(VertexList &set)
{
	for each(Vertex* v in set)
	{
		for each(Edge* e in v->edges) delete e;
		delete v;
	}
	set.clear();
}

// ----------------------------------------------------------------------------

// Разворачивание групп в множестве {{1,2}, {3}} --> {1, 2, 3}
	
VertexList extractGroups(const VertexList &set) 
{
	VertexList result;

	for each(Vertex* v in set)
	{
		if(v->group.size() > 0)
		{
			for each(Vertex* subv in v->group) result.push_back(subv);
		}
		else result.push_back(v);
	}

	return result;
}

// ----------------------------------------------------------------------------

// Создание матрицы смежности из множества

Matrix vertexListToMatrix(const VertexList &set)
{
	int n = static_cast<int>(set.size());
	Matrix m(n, Row(n, 0));

	// нумеруем вершины в поле flag
	// после чего заполняем матрицу
	for(int i = 0; i < n; i++) set[i]->flag = i;
	for each(Vertex* v in set)
	{
		for each(Edge* e in v->edges) m[v->flag][e->vertex->flag] = e->c;
	}
	
	return m;
}

// ----------------------------------------------------------------------------

// Сортировка списка по id вершин

void sortListById(VertexList &set)
{
	size_t n = set.size();

	for(size_t i = 0; i < n - 1; i++) 
	{
		size_t k = i;
		for(size_t j = i + 1; j < n; j++) 
		{
			if(set[j]->id < set[k]->id)	k = j;
		}
		if(k != i)
		{
			Vertex* v = set[k];
			set[k] = set[i];
			set[i] = v;
		}
	}
}

// ----------------------------------------------------------------------------

// строковое представление вершины для отчета

string vertexToStr(const Vertex* vertex)
{
	stringstream s;
	s << "{";
	if(vertex->group.size() == 0)
	{
		s << vertex->id << "}";
		return s.str();
	}
	else
	{
		for each(Vertex* v in vertex->group) s << vertexToStr(v) << ",";
		return s.str().substr(0, s.str().size() - 1) + "}";
	}
}

// ----------------------------------------------------------------------------

// строковое представление множества вершин для отчета

string vertexListToStr(const VertexList &set)
{
	stringstream s;
	s << "[ ";
	for each(Vertex* v in set) s << vertexToStr(v) << ", ";
	return s.str().substr(0, s.str().size() - 2) + " ]";
}

// ----------------------------------------------------------------------------

// строковое представление матрицы смежности множества вершин

string matrixToStr(const Matrix &m)
{
	int n = static_cast<int>(m.size());

	stringstream s;
	for(int i = 0; i < n; i++)
	{
		s << "    ";
		for(int j = 0; j < n; j++) s << setw(3) << m[i][j];
		s << endl;
	}
	return s.str();
}

// ----------------------------------------------------------------------------

// поиск кратчайшего пути из s в t
// возвращает НЕ путь, а очередь, образовавшуюся при поиске,
// которая понадобится для нахождения множества B

VertexList findPath(const VertexList &set, Vertex* s, Vertex* t)
{
	// очищаем пометки прошлого поиска
	for each(Vertex* v in set)
	{
		v->parent = NULL;
		v->flag = 0;
	}
	
	size_t i = 0;
	VertexList queue;

	// добавляем в очередь начальную вершину и отмечаем ее
	s->flag = 1;
	queue.push_back(s);

	// обрабатываем очередной элемент очереди
	while(i < queue.size())
	{
		// ставим в очередь все смежные, еще не побывавшие в 
		// очереди и имеющие запас веса, вершины (т.е. такие,
		// по которым можно проложить путь)
		for each(Edge* e in queue[i]->edges) 
		{
			Vertex* v = e->vertex;
			if(v->flag == 0 && e->c > 0) 
			{
				v->parent = queue[i];
				v->flag = 1;
				queue.push_back(v);

				// если смежная вершина = t - найден кратчайший путь
				if(v == t) return queue;
			}
		}
		// переходим к следующей в очереди вершине
		i++;
	}

	return queue;
}
// ----------------------------------------------------------------------------

// поиск max потока и его min сечения между вершинами 0 и 1 в множестве set

MinCut findMinCut(VertexList &set)
{
	MinCut result(set[0], set[1]);

	for(;;)
	{
		// находим кратчайший путь между s и t, 
		// если не найден - поток максимален
		VertexList path = findPath(set, result.s, result.t);
		if(result.t->parent == NULL) break;

		// начиная с t и продвигаясь по цепочке parent-ов пройдемся по
		// найденному пути: найдем минимальный c, на который можно 
		// увеличить поток по этому пути и соберем ребра этого пути
		int min_c = INT_MAX;
		Vertex* v = result.t;
		EdgeList edges;
		while(v->parent != NULL)
		{
			edges.push_back(getEdge(v, v->parent));
			Edge* e = getEdge(v->parent, v);
			edges.push_back(e);
			if(e->c < min_c) min_c = e->c;
			v = v->parent;
		}

		// увеличим поток и уменьшим вес на найденную величину
		result.f += min_c;
		for each(Edge* e in edges) 
		{
			e->f += min_c;
			e->c -= min_c;
		}
	}

	// находим B - половину разреза со стороны точки t, которая 
	// является очередью при поиске (неудачном) пути из t в s
	// вторая половина: A = set - B
	result.B = findPath(set, result.t, result.s);
	result.A = setMinus(set, result.B);

	// приводим в порядок веса после поиска
	for each(Vertex* v in set)
	{
		for each(Edge* e in v->edges)
		{
			e->c += e->f;
			e->f = 0;
		}
	}

	return result;
}

// ----------------------------------------------------------------------------

// Построение дерева Гомори-Ху

Matrix buildGomoryHuTree(const Matrix &g)
{
	// Шаг 1: Инициализация
	VertexList Vg = VertexList();
	VertexList Vt = VertexList();
	
	// создаем в Vg вершины и ребра по матрице смежности
	size_t n = g.size();
	for(size_t i = 0; i < n; i++)
	{
		Vg.push_back(new Vertex(static_cast<int>(i)));
		for(size_t j = 0; j < i; j++)
		{
			if(g[i][j] != 0) addEdge(Vg[i], Vg[j], g[i][j]);
		}
	}
	
	// в Vt помещаем 1 вершину, группирующую все вершины исходного графа
	Vt.push_back(new Vertex(Vg));

	cout << endl << "Step 1: Vt= " << vertexListToStr(Vt) << endl << endl;

	for(;;)
	{
		cout << "---------------------------------------------------------------" << endl << endl;
		
		// Шаг 2: Выбор группы вершин из Vt, в которой более 1 вершины
		// если все группы по одной вершине - дерево готово
		// иначе фиксируем выбранную группу в x

		VertexList::iterator i = Vt.begin();
		VertexList::iterator j = Vt.end();
		while(i != j && (*i)->group.size() < 2) i++;
		if(i == j) break;
		
		Vertex* x = *i;

		cout << "Step 2: X = " << vertexListToStr(x->group) << endl << endl;

		// Шаг 3: Строим граф G
		// Vt - это дерево. Значит от X отходят ОТДЕЛЬНЫЕ ветки, не связанные 
		// между собой иначе как через сам Х (т.к. в дереве нет циклов).
		// Каждую ветку от Х "сжимаем" в одну группу. А вот сам X расжимаем.
		// Было: http://upload.wikimedia.org/wikipedia/en/thumb/1/1c/Gomory–Hu_T3.svg/500px-Gomory–Hu_T3.svg.png (X = {1,4})
		// Стало: http://upload.wikimedia.org/wikipedia/en/thumb/6/60/Gomory–Hu_Gp4.svg/500px-Gomory–Hu_Gp4.svg.png
		// Впрочем, поскольку нельзя мешать ребра простых вершин исходного графа
		// с фиктивными ребрами между простыми вершинами и группами, то каждую
		// простую вершину тоже обернем в группу. Т.е. вершины {1}, {4} на картинке
		// выше так же как и свертки будут на самом деле {{1}}, {{4}}. Создание 
		// ребер между ними подчиняется общему алгоритму. Для упрощения создания
		// ребер воспользуемся свойством parent, которое у каждой простой вешины
		// будет указывать на группу, в которой она оказалась

		VertexList G = VertexList();

		// добавим вершины из X, оборачивая их в группы
		for each(Vertex* v in x->group) 
		{
			v->parent = new Vertex(VertexList(1, v));
			G.push_back(v->parent);
		}

		// добавим вершины-ветки
		for each(Edge* e in x->edges)
		{
			// Соберем все вершины ветки, начинающейся от ребра e в группу новой вершины z.
			// Для этого достаточно удалить ребро из e->vertex в x и найти несуществующий
			// после этого путь из e->vertex в x, результат развернуть, ребро создать заново
			deleteEdge(e->vertex, x);
			Vertex* z = new Vertex(extractGroups(findPath(Vt, e->vertex, x)));
			addEdge(e->vertex, x, e->c, false);

			for each(Vertex* v in z->group) v->parent = z;
			G.push_back(z);
		}

		// В каждой вершине G просматриваем ребра вершин ее группы и дублируем их "наверху".
		// Например, в группе вершины A есть простая вершина 1, из которой идет ребро в 
		// простую вершину 3, parent которой - групповая вершина B. Следовательно, проводим 
		// ребро между групповыми вершинами A и B. При этом, если уже такое ребро было, то
		// новый вес присуммируется к старому, таким образом, ребра между группами, как и 
		// полагается, будут иметь суммарный вес.
		for each(Vertex* z in G)
		{
			for each(Vertex* v in z->group)
			{
				for each(Edge* e in v->edges) 
				{
					// петли не делаем!
					if(z != e->vertex->parent) addEdge(z, e->vertex->parent, e->c, false);
				}
			}
		}
	
		cout << "Step 3: G = " << vertexListToStr(G) << endl << endl;
		cout << matrixToStr(vertexListToMatrix(G)) << endl << endl;

		// Шаг 4: Поиск min разреза с max потоком в G
		// s и t необходимо выбрать из вершин множества X, которое находится в начале
		// множества G. Для простоты findMinCut() выбирает вершины 0 и 1. 
		MinCut cut = findMinCut(G);
		cut.A = extractGroups(cut.A);
		cut.B = extractGroups(cut.B);

		cout << "Step 4: s-t   = " << vertexToStr(cut.s) << "-" << vertexToStr(cut.t) << endl;
		cout << "        max_f = " << cut.f << endl;
		cout << "        A     = " << vertexListToStr(cut.A) << endl;
		cout << "        B     = " << vertexListToStr(cut.B) << endl << endl;

		// удаляем динамически созданные элементы графа G
		deleteVertexList(G);

		// Шаг 5: Апдейтим дерево

		// X заменяется на X*A и X*B
		// X*A и X*B связываются ребром, равным макс потоку
		Vertex* XA = new Vertex(setMul(x->group, cut.A));
		Vertex* XB = new Vertex(setMul(x->group, cut.B));
		addEdge(XA, XB, cut.f);

		// X была (возможно) связана с некоторыми группами Vi
		// при этом Vi есть либо подмножеством A, либо подмножеством B
		// поэтому, если любой элемент из Vi найдется в A - значит Vi
		// есть подмож. А, тогда связываем Vi с X*A, иначе с X*B
		for each(Edge* e in x->edges)
		{
			Vertex* v = e->vertex; // Vi (v->group[0] - произв. элемент из Vi)
			addEdge((findVertex(cut.A, v->group[0]) != cut.A.end() ? XA : XB), v, e->c);
		}

		// вставляем X*A и X*B, убираем X
		Vt.insert(Vt.insert(i, XB), XA);
		deleteVertex(Vt, x);

		cout << "Step 5: Vt= " << vertexListToStr(Vt) << endl << endl;
		cout << matrixToStr(vertexListToMatrix(Vt)) << endl << endl;
	}

	// перемещаем вершины из подмножеств наверх
	for each(Vertex* v in Vt) 
	{
		v->id = v->group[0]->id;
		v->group.clear();
	}

	// возвращаем матрицу смежности отсортированого по id дерева
	// (чтобы порядок соответствовал исходной матрице)
	sortListById(Vt);
	Matrix m = vertexListToMatrix(Vt);

	cout << "Результат:" << endl << endl;
	cout << "Vt= " << vertexListToStr(Vt) << endl << endl;
	cout << matrixToStr(m) << endl << endl;

	// удаляем динамически созданные элементы Vg и Vt
	deleteVertexList(Vg);
	deleteVertexList(Vt);

	return m;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");
	
	// Проверяем, действительно ли задан не ориентированный граф
	for(int j = 0; j < sz; j++)
	{
		if(graph[j][j] != 0)
		{
			cout << "  Ошибка: петля в вершине " << j << endl;
			return 0;
		}

		for(int i = 0; i < j; i++)
		{
			if(graph[i][j] != graph[j][i])
			{
				cout << "  Ошибка: отличаются веса ребра " << i << "-" << j << endl;
				return 0;
			}

			if(graph[i][j] < 0)
			{
				cout << "  Ошибка: отрицательный вес ребра " << i << "-" << j << endl;
				return 0;
			}
		}
	}

	Matrix g(sz, Row(sz));
	for(int i = 0; i < sz; i++) for(int j = 0; j < sz; j++) g[i][j] = graph[i][j];
	buildGomoryHuTree(g);
	return 0;
}

// ----------------------------------------------------------------------------
