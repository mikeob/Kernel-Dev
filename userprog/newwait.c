int
process_wait (tid_t child_tid)
{
  int status = 0;
  struct list_elem *e;
  
  struct thread *cur = thread_current();
  struct exit *ex = NULL;
  
  /* Check to see if child_tid is an actual child of thread_current */ 
  for (e = list_begin (cur->exit_list); e != list_end(cur->exit_list);
        e = list_next(e)) 
      {
          ex = list_entry(f, struct exit, elem);
          if (ex->parent = cur->tid && ex->child = child_tid)
          {
            list_remove(ex->elem);
            break;  
          }
          ex = NULL;
      }

  /* child_tid not valid */
  if (ex == NULL)
  {
    return -1;
  }

  /* Wait if necessary */
  sema_down(ex->sema);

  int ans = ex->status;
  free(ex);

  return ans;
}



