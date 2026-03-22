
//-----------------------------------------------------------------------------
//
//  Name:   IndexedPriorityQueue.h
//
//  Desc:   인덱스 기반 최소 이진 힙 (IndexedPriorityQLow)
//          TimeSlicedGraphAlgorithms 에서 사용하는 우선순위 큐.
//          외부 키 배열을 참조하며 ChangePriority() 로 키 갱신을 지원한다.
//
//          원본: Mat Buckland (misc/PriorityQueue.h) - 의존성 없는 독립 구현
//
//-----------------------------------------------------------------------------
#include "pch.h"


//-----------------------------------------------------------------------------
// IndexedPriorityQLow
//
// 키 값이 낮을수록 높은 우선순위를 갖는 인덱스-기반 최소 힙.
// m_vKeys 는 외부 배열의 참조이며, 힙은 해당 배열의 인덱스를 저장한다.
//
// 인터페이스:
//   insert(idx)         - 인덱스를 힙에 삽입
//   Pop()               - 최솟값 인덱스를 반환하며 제거
//   ChangePriority(idx) - 인덱스 idx 의 키가 감소했을 때 힙 재정렬
//   empty()             - 힙이 비었는지 반환
//-----------------------------------------------------------------------------





template <class KeyType>
class IndexedPriorityQLow
{
private:

    std::vector<KeyType>& m_Keys;     // 외부 키 배열 (참조)
    std::vector<int> m_Heap;      // 1-indexed 힙 (인덱스 저장)
    std::vector<int> m_InvHeap;   // 역 힙: m_InvHeap[nodeIdx] = 힙 위치
    int m_nSize;
    int m_nMaxSize;

    // 두 힙 슬롯을 교환하고 역 힙도 갱신
    void Swap(int a, int b)
    {
        int temp = m_Heap[a];
        m_Heap[a] = m_Heap[b];
        m_Heap[b] = temp;
        m_InvHeap[m_Heap[a]] = a;
        m_InvHeap[m_Heap[b]] = b;
    }

    // 위쪽으로 재정렬 (삽입 / ChangePriority 후)
    void ReorderUpwards(int nd)
    {
        while (nd > 1 && m_Keys[m_Heap[nd / 2]] > m_Keys[m_Heap[nd]])
        {
            Swap(nd / 2, nd);
            nd = nd / 2;
        }
    }

    // 아래쪽으로 재정렬 (Pop 후)
    void ReorderDownwards(int nd, int HeapSize)
    {
        while (2 * nd <= HeapSize)
        {
            int nChild = 2 * nd;

            if (nChild < HeapSize &&
                m_Keys[m_Heap[nChild]] > m_Keys[m_Heap[nChild + 1]])
            {
                nChild++;
            }

            if (m_Keys[m_Heap[nd]] > m_Keys[m_Heap[nChild]])
            {
                Swap(nd, nChild);
                nd = nChild;
            }
            else
            {
                break;
            }
        }
    }


public:

    IndexedPriorityQLow(std::vector<KeyType>& keys, int MaxSize)
        : m_Keys(keys)
        , m_nMaxSize(MaxSize)
        , m_nSize(0)
    {
        m_Heap.assign(MaxSize + 1, 0);
        m_InvHeap.assign(MaxSize, 0);
    }

    bool empty() const { return m_nSize == 0; }

    void insert(int idx)
    {
        assert(idx >= 0 && idx < m_nMaxSize);
        ++m_nSize;
        m_Heap[m_nSize]  = idx;
        m_InvHeap[idx]   = m_nSize;
        ReorderUpwards(m_nSize);
    }

    // 최솟값 인덱스를 반환하며 힙에서 제거
    int Pop()
    {
        Swap(1, m_nSize--);
        ReorderDownwards(1, m_nSize);
        return m_Heap[m_nSize + 1];
    }

    // 키 값이 감소한 인덱스를 위쪽으로 재정렬
    void ChangePriority(int idx)
    {
        ReorderUpwards(m_InvHeap[idx]);
    }
};
