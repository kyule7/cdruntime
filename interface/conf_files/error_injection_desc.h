/**addtogroup example_error_injection 
 * @{
 */

/**@def ERROR_INJECTION
 * @brief Description for error injection.
 *
 * There are three options to inject error to CDs. 
 * If CDs are mapped to system hierarchy, a failure mode between tasks in the same CD will be correlated.
 * For example, many tasks in the same CD are physically running at the same rack,
 * and there is a power failure at that rack for some reason,
 * all the tasks corresponding to the CD will get failed.
 * This error injection reflect this kind of error event that has strong correlations between tasks in the CD.
 *
 * Option 1. INJECT_ERROR_LV_X
 *        This option injects error for whole tasks at level X.
 *        If the XXX_RATE is set to 1, it always forces whole tasks failed at level X.
 *
 * Option 2. INJECT_ERROR_RANK_IN_LV_X
 *        This option injects error for all the tasks in a CD at level X.
 *        The rank at level X can be set by this flag.
 *        If the XXX_RATE is set to 1, it always forces all the tasks of a CD failed at level X.
 *
 * Option 3. INJECT_ERROR_LV_X
 *        This option injects error for a single task at level X.
 *        If the XXX_RATE is set to 1, it always forces the task failed at level X.
 */

#define INJECT_ERROR_LV_1 1
#define INJECT_ERROR_LV_1_RATE 1
#define INJECT_ERROR_RANK_IN_LV1 5
#define INJECT_ERROR_RANK_IN_LV1_RATE 1
#define INJECT_ERROR_TASK_AT_LV1 2
#define INJECT_ERROR_TASK_AT_LV1_RATE 2

#define INJECT_ERROR_LV_2 1
#define INJECT_ERROR_RANK_IN_LV_2 2
#define INJECT_ERROR_TASK_AT_LV_1 2

#define INJECT_ERROR_LV_3 1
#define INJECT_ERROR_RANK_IN_LV_3 2
#define INJECT_ERROR_TASK_AT_LV_1 2

#define INJECT_ERROR_LV_4 1
#define INJECT_ERROR_RANK_IN_LV_3 1
#define INJECT_ERROR_TASK_AT_LV_1 2

#define INJECT_ERROR_LV_5 1
#define INJECT_ERROR_RANK_IN_LV_3 1
#define INJECT_ERROR_TASK_AT_LV_1 2

/** @} */
