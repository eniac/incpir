#include<unistd.h>
#include<stdio.h>
#include<pthread.h>

int book=0;//给book中写数据
pthread_rwlock_t rwlock;//定义读写锁

// client put request into the vector every 5 sec.
// measure how long it takes for each individual request complete in this function
void *myread(void *arg)
{   
    sleep(1);//写者先运行
    while(1)
    {  
        if(pthread_rwlock_tryrdlock(&rwlock)!=0)
        {
            printf("reader say:the wirter is writing...\n");
            sleep(1);
        }
        else
        {
        // put request
        // while 等待 直到拿到reply (reply和request要有个标记来区别)
        // 记录时间
        printf("read done...%d\n",book);

        pthread_rwlock_unlock(&rwlock);

        }
    }

}

// writer is server, fetch a request from the vector, then put it back
void *mywirte(void *arg)
{

     while(1)
     {  
         if(pthread_rwlock_trywrlock(&rwlock)!=0)
        {
           printf("writer say:the reader is reading...\n");

        }
        else
        {
         // server takes out a task from vector
         // server process
         // server put back a task from client
         book++;
         printf("write done...%d\n",book);
         sleep(1); // write 
         pthread_rwlock_unlock(&rwlock);

        }
     }

}


int main()
{
   pthread_rwlock_init(&rwlock,NULL);//初始化读写锁

   pthread_t reader,wirter;

   pthread_create(&reader,NULL,myread,NULL);
   pthread_create(&wirter,NULL,mywirte,NULL);

   pthread_join(reader,NULL);
   pthread_join(wirter,NULL);

   pthread_rwlock_destroy(&rwlock);//销毁读写锁

   return 0;


}
