/*                                                           */
/*  LINKED1.C  :   Simple Linked list example                */
/*                                                           */
/*                  Programmed By Lee jaekyu                 */
/*                                                           */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>


typedef struct _log_data
{
  int state;
  struct timeval tv;
}log_data;

typedef struct _node
{
  log_data data;  //정보 저장
  struct _node *next; // 다음노드 위치 저장
} node;

node *head, *tail;

// 리스트를 초기화 합니다.
void init_list(void)
{
  head = (node*)malloc(sizeof(node)); // 머리공간 확보
  tail = (node*)malloc(sizeof(node)); // 꼬리공간 확보
  head->next = tail;  // 머리 다음은 꼬리
  tail->next = tail;  // 꼬리의 다음은 꼬리
}

// 리스트의 첫번째 항목을 삭제합니다.
int delete_first_node (void)
{
  node *next_data;
  node *now_data;

  now_data = head;
  next_data = now_data->next;

  if (next_data != tail)  /* if find */
    {
      now_data->next = next_data->next;
      free(next_data);
      return 1;
    }
  else
    return 0;
}


// 현재 리스트가 오름차순으로 정렬되어 있다고 가정하고,
// 정렬이 깨지지 않도록 순거에 맞게 노드를 삽입하는 함수
void ordered_insert(log_data *pData)
{
  node *next_data;
  node *now_data;
  node *new_data;
  int count = 0;

  now_data = head;
  next_data = now_data->next;

  while (next_data != tail)
    {
      now_data = now_data->next;
      next_data = now_data->next;
      count++;
    }

  new_data = (node*)malloc(sizeof(node));
  new_data->data.state = pData->state;
  new_data->data.tv = pData->tv;

  now_data->next = new_data;
  new_data->next = next_data;
  
  printf ("#J: list counter = %d\n", count);
  if (count > 5)
    delete_first_node ();

  return;
}


void print_list(node* t)
{
  int count = 0;
  printf("\n\n");

  while (t != tail)
    {
      printf("%d] %-8d  %ld \r\n", count, t->data.state, t->data.tv.tv_sec);
      t = t->next;
    }

  printf("\n");
}

node *delete_all(void)
{
    node *s;
    node *t;
    t = head->next;
    while (t != tail)
        {
        s = t;
        t = t->next;
        free(s);
        }
    head->next = tail;
    return head;
}

int main(void)
{
  int i = 0;
  int count = 0;
  log_data local_data;

  init_list();
  printf("\nInitial Linked list is ");

  for (i = 0;i < 10;i++)
    {
      local_data.state = count++;
      gettimeofday(&local_data.tv, NULL);

      ordered_insert(&local_data);
      print_list(head->next);
    }

  return 0;

#if 0
    printf("\nInitial Linked list is ");
    print_list(head->next);

    printf("\nFinding 4 is %ssuccessful", find_node(4) == tail ? "un" : "");

    t = find_node(5);
    printf("\nFinding 5 is %ssuccessful", t == tail ? "un" : "");

    printf("\nInserting 9 after 5");
    insert_after(9, t);
    print_list(head->next);

    t = find_node(10);
    printf("\nDeleting next last node");
    delete_next(t);
    print_list(head->next);

    t = find_node(3);
    printf("\nDeleting next 3");
    delete_next(t);
    print_list(head->next);

    printf("\nInsert node 2 before 3");
    insert_node(2, 3);
    print_list(head->next);

    printf("\nDeleting node 2");
    if (!delete_node(2))
        printf("\n  deleting 2 is unsuccessful");
    print_list(head->next);

    printf("\nDeleting node 1");
    delete_node(1);
    print_list(head->next);

    printf("\nDeleting all node");
    delete_all();
    print_list(head->next);
#endif
}



