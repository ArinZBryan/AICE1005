# Decision Tree
Your job is to implement a decision tree in C++. This is a learning machine that makes decisions
based on a set of rules that form a binary tree. The data consists of a number of features that
may be categorical (e.g. leg colour: red, yellow, green) or numerical (e.g. weight: 0.75kg)
as well as a class (e.g. bird type: common gull, black-headed gull, etc.). Decision trees are 
descibed in [wikipedia](https://en.wikipedia.org/wiki/Decision_tree_learning) and various 
[blogs](https://www.geeksforgeeks.org/decision-tree-introduction-example/). An implementation 
(which you might wish to test against) is available in 
[scikit-learn](https://scikit-learn.org/stable/modules/tree.html). To make your job easier try
this on this very simple 
[training data set](https://ecs-vlc.github.io/aice1005/decisionTrees/training.dat). I also 
provide a [test set](https://ecs-vlc.github.io/aice1005/decisionTrees/testing.dat). Each row
has ten binary features (0 or 1) and one class label (0, 1 or 2). You want to build a decision
tree classifier to predict the class label from the features. Note that I am only asking you to
implement a decision tree for classification (they can also be used for regression).

You may use a chatbot to help you code, but you should detail the process (i.e. prompts) you 
used to get the thing to work. If you use a chatbot you should also discuss how you went 
about verifying that the code worked as intended.

This is a challenging coursework and marks will be given if you do not succeed as long as you 
describe the work that you have done.

The coursework is due on on **Thursday 8th May at 4pm**. You should submit a maximum of two page
PDF outlining your work. This should discuss the code you implemented (at a high level) and the
test you carried. You should also submit you code in a second file (if you have written multiple 
files **as you should** just concatenate them into a single file).

### Implementing a Decision Tree
- Your code should read a data file consisting of a list of features and a class label for each 
  data point ([training.dat](https://ecs-vlc.github.io/aice1005/decisionTrees/training.dat)).
  You may assume the first ten items on each line corresponds to a binary feature and the last 
  item the class of the data set (there are three classes).
- You should build a binary tree of rules that separate the data into the leaves of the tree. 
  The tree consists of two types of nodes (rules and leaves). The rules describe a rule for 
  splitting the data. The leaves hold the data after splitting (or at least an identifier for 
  each data point). The rules should only depend on a single category (e.g. if leg colour is 
  red go left, or if weight is greater than 0.56 go left). Note that you can write a `LeafNode` 
  class and a `RuleNode` class both of which inherit from a `Node` class. Both child classes can
  be pointed to by a pointer of type `Node*`.
- Each time you split a leaf you need to compute a "purity" score for splitting into two new
  leaves. The purity score is the maximum improvement in purity you get by splitting on one of 
  the features. To measure the purity you measure the proportion of data points associated with
  a leaf in each class. You can then use either the gini or entropy score (see web pages for 
  details) to compute the purity.
- Once you have computed a purity improvement score you need to add it to a priority queue (you
  need to add a pointer to the leaf). You minimise purity scores so your purity improvement 
  should be negative. Then you can use a min-heap.
- You then pop from the priority queue to decide which leaf node to split next.
- You should use the standard template library for classes such as priority queue and vector 
  (but you will need to write your own binary tree).
- You will find it much easier to write a number of auxiliary classes that keep together 
  information you need.
- You should stop after a set number of nodes (rules) have been produced.
- You can test your performance on the [test set](https://ecs-vlc.github.io/aice1005/decisionTrees/testing.dat)
  as a function of the number of rules in the decision tree. Present the results in final report.
### Extensions
These extensions are optional. If you do a very good job on the task above you can get full marks.
- Once you have a decision tree you can implement a random forest algorithm. This consists of
  using many decision trees and then getting them to vote on class. To make the decision trees
  different, you allow each tree to only split the data depending on a different subset of the
  features (e.g. 3 or 4) features. The trees can be quite shallow (2-10 rules), but you average
  over a lot of trees. To implement this you would need to add a random binary mask to each tree
  indicating which features the tree is allowed to use. Measure the performance on the test data
  for your random forest.
- You might want to extend this to work on real data (e.g. this 
  [data set](https://www.kaggle.com/datasets/deadier/play-games-and-success-in-students)). Here
  the job would be to identify the type of each feature. If the feature is categorical then you
  have to split on categories. If it is numerical you have to split on all possible splits of the
  data (you can speed this up by sorting the data according to value of the feature).
- You might have some features with missing data. At each node you have an additional rule to 
  decide whether missing data should go left or right. You make this decision based on which one
  improves the purity the most.

[Adam Prugel Bennett](apb@ecs.soton.ac.uk)