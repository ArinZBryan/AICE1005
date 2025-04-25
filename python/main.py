from sklearn import tree
import matplotlib.pyplot as plt

class DataFrame:
    def __init__(self):
        self.label = ""
        self.fields = []
    def __repr__(self):
        return f"{self.label} {self.fields}"

file = open("C:\\Users\\arinb\\OneDrive\\Documents\\Computer_Engineering\\Coursework\\AICE1005\\training.dat", "r")
label_index = 10

lines: list[str] = file.readlines()
ret: list[DataFrame] = []

for line in lines:
    df: DataFrame = DataFrame()
    tokens = line.split(" ")
    for i in range(len(tokens)):
        if i == label_index:
            df.label = tokens[i].strip()
        elif "." in tokens[i]:
            df.fields.append(float(tokens[i]))
        elif tokens[i].isalpha():
            df.label = tokens[i].strip()
        else:
            df.fields.append(int(tokens[i]))
    ret.append(df)

for df in ret:
    for field in df.fields:
        if field == 0:
            field = -1


field_arr = list(map(lambda x: x.fields, ret))
label_arr = list(map(lambda x: x.label, ret))

dtree = tree.DecisionTreeClassifier(criterion="entropy", max_depth=3)
dtree = dtree.fit(field_arr, label_arr)
plt.figure(dpi=300)
tree.plot_tree(dtree, class_names=True, impurity=True)
plt.show()
file.close()