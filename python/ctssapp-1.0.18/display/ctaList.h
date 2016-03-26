#ifndef CTALIST_H
#define CTALIST_H

#define CTALIST_INIT(name) { &name, &name }

// --------------------------------------------------------------------------
// CTA_LIST functions never allocate or delete anything. They just manages the
// list.
// --------------------------------------------------------------------------

typedef struct ctalist
{
    struct ctalist *next;
    struct ctalist *prev;
    void *data;
} CTA_LIST;


// --------------------------------------------------------------------------

static inline void ctalistInit (CTA_LIST *lst)
{
    lst->next = lst;
    lst->prev = lst;
}

// --------------------------------------------------------------------------

static inline void ctalistAdd (CTA_LIST *prev, CTA_LIST *new, CTA_LIST *next)
{
    next->prev = new;
    new->next  = next;
    prev->next = new;
    new->prev  = prev;
}

// --------------------------------------------------------------------------

static inline void ctalistAddFront (CTA_LIST *head, CTA_LIST *new)
{
    ctalistAdd (head, new, head->next);
}

// --------------------------------------------------------------------------

static inline void ctalistAddBack (CTA_LIST *head, CTA_LIST *new)
{
    ctalistAdd (head->prev, new, head);
}

// --------------------------------------------------------------------------

static inline void ctalistDel (CTA_LIST *prev, CTA_LIST *next)
{
    next->prev = prev;
    prev->next = next;
}

// --------------------------------------------------------------------------

static inline int ctalistIsEmpty (CTA_LIST *head)
{
    return head->next == head;
}

// --------------------------------------------------------------------------


static inline CTA_LIST *ctalistGetNext (CTA_LIST *entry)
{
    if (ctalistIsEmpty (entry))
        return 0;

    return entry->next;
}

// --------------------------------------------------------------------------

static inline void ctalistDelEntry (CTA_LIST *entry)
{
    if (entry->next == entry)
    {
	printf ("ctalistDelEntry: entry->next == entry, returning\n");
	return;
    }

    ctalistDel (entry->prev, entry->next);
    ctalistInit (entry); // dissociate from any list.
}

// --------------------------------------------------------------------------

static inline void dumpList (CTA_LIST *head)
{
    CTA_LIST *tmp = head;
    printf ("dumpList:\n");
    do
    {
	printf ("   node = %p\n", tmp);
	tmp = tmp->next;
    } while (tmp != head);
}

#endif
